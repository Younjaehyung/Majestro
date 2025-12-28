#include "pch.h"
#include "RenderSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Material.h"
#include "World.h"
#include "RenderComponent.h"
#include "RenderTarget.h"
#include "CameraComponent.h"
#include "AnimationComponent.h"
#include "ParticleComponent.h"
#include "TagComponent.h"
#include "TerrainComponent.h"


RenderSystem::RenderSystem(World* world) : System::System(world)
{
	mCamera = nullptr;
}

void RenderSystem::Initialize()
{
	mRenderComponentPool = &(mWorld->GetComponentPool<RenderComponent>());
	mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");

	// immutability Data
	PushMaterialData();
	

	mDeferredDrawItems.reserve(1000);
	mDeferredDrawBatchs.reserve(1000);
	mInstanceVector.reserve(1000);
}

void RenderSystem::Update()
{

	std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>()[0]};
	mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);
	
	ClearRTV();

	PushData();

	DefferdRendering();

	ForwardRendering();

	//ParticleRendering();

	


}


void RenderSystem::PushData()
{
	RENDERMANAGER.SetGraphicsTable();
	PushLandData();
	PushPassData();
	PushObjectData();
	PushLightData();
	PushInstanceData();
}

void RenderSystem::DefferdRendering()
{
	RenderShadow();

	RenderGBuffer();

	RenderLights();

	RenderFinal();
}

void RenderSystem::ForwardRendering()
{
	RenderForward();
}

void RenderSystem::ParticleRendering()
{
	RenderingParticle();
}


void RenderSystem::ClearRTV()
{
	// SwapChain Group 초기화
	uint8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).ClearRenderTargetView(backIndex);

	// Shadow Group 초기화 
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).ClearRenderTargetView();

	// Deferred Group 초기화 
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).ClearRenderTargetView();

	// Lighting Group 초기화 
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).ClearRenderTargetView();


	mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

	ClearBuffer();
	// 추후 lightvector관련들 clear도 모두 확인할껏.
}

void RenderSystem::ClearBuffer()
{
	mCurrPSOID = 0;

	mDeferredDrawItems.clear();
	mDeferredDrawBatchs.clear();
	mInstanceVector.clear();
	//passParams = {};

	
	mLightVector.clear();
	mObjectVector.clear();
	mDummyVector.clear();

}



void RenderSystem::PushPassData()
{
	
	passParams.MatView = mCamera->mView.Transpose();
	passParams.MatProjection = mCamera->mProjection.Transpose();
	passParams.MatViewInv = mCamera->mView.Invert().Transpose();
	passParams.MatProjectionInv = mCamera->mProjection.Invert().Transpose();
	passParams.ScreenSize = { static_cast<float>(RENDERMANAGER.GetWindow().Width), static_cast<float>(RENDERMANAGER.GetWindow().Height) };
	
	//passParams.TotalTime = TIME.GetTotalTime();


	shared_ptr<GroupBuffer> groupBuffer = RENDERMANAGER.GetGroupBuffer(mFrameCount);
	groupBuffer->PassInfo->PushData(&passParams, sizeof(PassParams));

}

void RenderSystem::PushMaterialData()
{

	uint32 index{};
	for (auto& materials : RESOURCEMANAGER.GetAllResources<Material>()) {
		shared_ptr<Material> material = dynamic_pointer_cast<Material>(materials.second);
		material->SetIndex(index);
		mMaterialVector.emplace_back(material->GetParams());
		index++;
	}
	RENDERMANAGER.GetMaterialBuffers()->PushDefaultToData(mMaterialVector.data(), static_cast<uint32>(mMaterialVector.size() * sizeof(MaterialParams)));

}

void RenderSystem::PushLandData()
{
	auto entity = mWorld->GetEntitiesWithComponent<TerrainComponent>();

	auto terrain = mWorld->GetComponent<TerrainComponent>(entity[0])->mTerrainParams;

	passParams.HeightMapResolution = terrain.HeightMapResolution;
	passParams.MaxTessLevel = terrain.MaxTessLevel;
	passParams.MinMaxTessDistance = terrain.MinMaxTessDistance;
	passParams.TileCountX = terrain.TileCountX;
	passParams.TileCountZ = terrain.TileCountZ;


}

void RenderSystem::PushInstanceData()
{
	RENDERMANAGER.GetGroupBuffer(mFrameCount)->InstanceInfo->PushGraphicsData(mInstanceVector.data(), static_cast<uint32>(mInstanceVector.size() * sizeof(RenderParams)));
}




void RenderSystem::PushLightData()
{
	// light Component 추출
	const vector<Entity>& entities = mWorld->GetEntitiesWithComponent<LightComponent>();
	ComponentPool<LightComponent>& lightComponents = mWorld->GetComponentPool<LightComponent>();

	// push 시작 (vector에 값 밀어 넣기)
	for (auto& entity : entities)
	{
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		LightComponent* lightComponent = mWorld->GetComponent<LightComponent>(entity);
		RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);


		lightParams = {};

		lightParams.Color = lightComponent->mLightInfo.Color;
		lightParams.Position = lightComponent->mLightInfo.Position;
		lightParams.Direction = lightComponent->mLightInfo.Direction;
		lightParams.LightType = lightComponent->mLightInfo.LightType;
		lightParams.Range = lightComponent->mLightInfo.Range;
		lightParams.Angle = lightComponent->mLightInfo.Angle;

		lightParams.MatWorld = transformComponent->mWorldMatrix.Transpose();;
		lightParams.MatView = cameraComponent->mCameraParams.MatView.Transpose();
		lightParams.MatProjection = cameraComponent->mCameraParams.MatProjection.Transpose();
		lightParams.MatViewInv = cameraComponent->mCameraParams.MatViewInv.Transpose();
		lightParams.MatProjectionInv = cameraComponent->mCameraParams.MatProjectionInv.Transpose();

		mLightVector.push_back(lightParams);

	}		
	RENDERMANAGER.GetGroupBuffer(mFrameCount)->LightInfo->PushGraphicsData(mLightVector.data(), static_cast<uint32>(sizeof(LightParams)*mLightVector.size()));
	
}

void RenderSystem::PushObjectData()
{
	const vector<EntityID>& gameObjects = mRenderComponentPool->GetEntities();
	
	uint32 index{};
	int32 index2{};
	for (const EntityID& gameObject : gameObjects)		// 같은 머테리얼을 가진 것끼리 분류
	{
		if (mWorld->GetComponent<LightComponent>(gameObject)) {
			continue;
		}
		

		TransformComponent*		transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
		RenderComponent*		renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);
		AnimationComponent*		animationComponent = mWorld->GetComponent<AnimationComponent>(gameObject);

		

		objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
		mObjectVector.push_back(objectParams);		// 트랜스폼 갱신
			
		renderComponent->mObjectIndex = index++;	// objectParams의 index 지정
		
		index2 = animationComponent ? animationComponent->mAnimInstanceID : -1;
		


		uint32 subMaterialIdx{};
		for (shared_ptr<Material>& material : renderComponent->mMaterials) {
			mDeferredDrawItems.emplace_back(
				material->GetShader(),
				renderComponent->mMesh,
				material->GetShaderID(),
				renderComponent->mMesh->GetID(),
				material->GetID(),
				subMaterialIdx++,
				RenderParams{ renderComponent->mObjectIndex, material->GetIndex(),index2,0 }
			);
		}
	}



	std::sort(mDeferredDrawItems.begin(), mDeferredDrawItems.end(), [](auto& a, auto& b) {
		if (a.PSOID != b.PSOID) return a.PSOID < b.PSOID;   // PSO 우선
		if (a.MeshID != b.MeshID) return a.MeshID < b.MeshID;  // Mesh
		return a.SubMesh < b.SubMesh;                          // Submesh
		});


	uint32 psoId{};
	uint32 meshId{};
	uint32 smIdx{};
	uint32 base{};

	for (uint32 i = 0; i < mDeferredDrawItems.size(); ) {
		psoId = mDeferredDrawItems[i].PSOID;
		meshId = mDeferredDrawItems[i].MeshID;
		smIdx = mDeferredDrawItems[i].SubMesh;

		// 이 배치의 인스턴스들을 전역 InstanceGPU[]에 연속으로 복사
		base = (uint32)mInstanceVector.size();//-i;	// 수식이 이게 맞나? 확인 필요
		uint32 j = i;

		while (j < mDeferredDrawItems.size()
			&& mDeferredDrawItems[j].PSOID == psoId
			&& mDeferredDrawItems[j].MeshID == meshId
			&& mDeferredDrawItems[j].SubMesh == smIdx) 
		{
			mInstanceVector.push_back(mDeferredDrawItems[j].InstanceGPU);	
			++j;
		}


		mBatch.PSOShader = mDeferredDrawItems[i].PSOShader;
		mBatch.Mesh = mDeferredDrawItems[i].PMesh;
		mBatch.PSOID = psoId;
		mBatch.MeshID = meshId;
		mBatch.SubMesh = smIdx;
		mBatch.SubMeshIndex = mDeferredDrawItems[i].SubMeshIndex;
		mBatch.BaseInstance = base;
		mBatch.InstanceCount = (uint32)(j - i);    // 이 run의 인스턴스 수
		//mBatch.InstanceGPU = mDeferredDrawItems[i].InstanceGPU;
		//mBatch.ParamsINX = i;
		mDeferredDrawBatchs.push_back(mBatch);

		i = j; // 다음 run으로
	}




	RENDERMANAGER.GetGroupBuffer(mFrameCount)->ObjectInfo->PushGraphicsData(mObjectVector.data(), static_cast<uint32>(sizeof(objectParams)*mObjectVector.size()));
}




void RenderSystem::RenderShadow()
{

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).OMSetRenderTargets();
	LightComponent* lightComponent;
	CameraComponent* cameraComponent;
	RenderComponent* renderComponent;

	for (auto& light : mWorld->GetEntitiesWithComponents<LightComponent,CameraComponent,RenderComponent>())
	{
		lightComponent = mWorld->GetComponent<LightComponent>(light);
		cameraComponent = mWorld->GetComponent<CameraComponent>(light);
		renderComponent = mWorld->GetComponent<RenderComponent>(light);
		if (lightComponent->mLightInfo.LightType != static_cast<int32>(LIGHT_TYPE::DIRECTIONAL_LIGHT))
			continue;

		RenderShadowCamera(light, lightComponent, cameraComponent, renderComponent);
	}


	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).WaitTargetToResource();
}

void RenderSystem::RenderGBuffer()
{
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).OMSetRenderTargets();

	struct dummy { uint32 BaseInstance; uint32 InstanceCount; } dum;

	for(auto& drawBatch : mDeferredDrawBatchs ) 
	{
		if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::DEFERRED)
			continue;

		if (mCurrPSOID != drawBatch.PSOID) {
			drawBatch.PSOShader->Update();
			mCurrPSOID = drawBatch.PSOID;
		}
		dum.BaseInstance = drawBatch.BaseInstance;
		dum.InstanceCount = drawBatch.InstanceCount;

		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0,2,&(dum),0);
		InstancingRender(drawBatch);
	}

	
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).WaitTargetToResource();

}

void RenderSystem::RenderLights()
{
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).OMSetRenderTargets();


	//// 광원을 그린다.
	//// 광원을 기준으로 나머지 객체들을 그린다

	//for (auto& light : _lights)
	//{
	//	light->Render();
	//}

	auto lights = 
	mWorld->GetEntitiesWithComponents<LightComponent, TransformComponent, CameraComponent, RenderComponent>();

	for(auto& light : lights)
	{
		
		RenderComponent* lightComponent =  mWorld->GetComponent<RenderComponent>(light);

		RESOURCEMANAGER.Get<Shader>(lightComponent->mMaterials[0]->GetShaderName())->Update();


		lightComponent->mMesh->Render();
	}
	////리소스에서 타켓으로
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).WaitTargetToResource();

}

void RenderSystem::RenderFinal()
{
	// Swapchain OMSet
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);

	
	RESOURCEMANAGER.Get<Shader>(L"Final")->Update();
	
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
}

void RenderSystem::RenderForward()
{
	for (auto& drawBatch : mDeferredDrawBatchs)
	{
		if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD)
			continue;

		if (mCurrPSOID != drawBatch.PSOID) {
			drawBatch.PSOShader->Update();
			mCurrPSOID = drawBatch.PSOID;
		}
		dum.BaseInstance = drawBatch.BaseInstance;
		dum.InstanceCount = drawBatch.InstanceCount;

		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 2, &(dum), 0);
		InstancingRender(drawBatch);
	}

}

void RenderSystem::RenderingParticle()
{

	//for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::PARTICLE)]) {

	//	RESOURCEMANAGER.Get<Shader>(shaderID)->Update();


	//	for (auto& particle : vec)
	//	{
	//		ParticleComponent* particleComponent = mWorld->GetComponent<ParticleComponent>(particle);
	//		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(particle);



	//		particleComponent->_mesh->Render(particleComponent->_maxParticle);
	//	}
	//}

}

bool RenderSystem::IsFrustumCulled()
{
	

	return false;
}

void RenderSystem::RenderShadowCamera(Entity& light , LightComponent* lightComponent, CameraComponent* cameraComponent, RenderComponent* renderComponent)
{
	//TransformComponent* transformComponent;
	//RenderComponent* objectRenderComponent;
	//
	//
	//RESOURCEMANAGER.Get<Shader>(L"Shadow")->Update();

	//for ( const auto& gameObjects : mMaterialObjectBatch)
	//{
	//	if (gameObjects.second.empty()) {
	//		continue;
	//	}

	//	for (const auto& gameObject : gameObjects.second) {

	//		

	//		transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
	//		objectRenderComponent = mWorld->GetComponent<RenderComponent>(gameObject);


	//		//if (gameObject->IsStatic())	//정적 물체인지 동적물체인지 확인해서 그림자 최적화
	//		//	continue;


	//		//if (IsCustomCulled(renderComponent->GetLayerIndex()))
	//		//	continue;

	//		if (objectRenderComponent->mCheckFrustum)
	//		{
	//			if (cameraComponent->mFrustum.ContainsSphere(
	//				transformComponent->GetWorldPosition(),
	//				transformComponent->GetBoundingSphereRadius()) == false)
	//			{
	//				continue;
	//			}

	//		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        

	//		mDummyVector.push_back(gameObject);

	//	}

	//	if (mDummyVector.empty()) {
	//		continue;
	//	}

	//	//InstancingRender(mDummyVector);

	//}

}

void RenderSystem::InstancingRender(DrawBatch& drawBatch)
{
	

	drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex, 0,0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
	
}

