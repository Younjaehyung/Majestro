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


RenderSystem::RenderSystem(World* world) : System::System(world)
{
	mCamera = nullptr;
}

void RenderSystem::Initialize()
{
	// 1번 초기화
	mRenderComponentPool = &(mWorld->GetComponentPool<RenderComponent>());
	mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");

	// immutability Data
	//PushMaterialData();

}

void RenderSystem::Update()
{

	std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);

	//ClearRTV();

	//SetTable();

	PushData();

	//DefferdRendering();

	//ForwardRendering();

	//ParticleRendering();



}


void RenderSystem::PushData()
{

	PushPassData();
	//PushGBufferData();
	//PushObjectData();
	//PushLightData();

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



	ClearBuffer();
	// 추후 lightvector관련들 clear도 모두 확인할껏.
}

void RenderSystem::ClearBuffer()
{

	for (auto& shaderGroup : shaderBatches) {

		for (auto& [shaderID, vec] : shaderGroup) {
			vec.clear(); // vector의 capacity는 유지됨
		}
	}

	for (auto& [ID, vec] : mMaterialObjectBatch) {
		vec.clear();
	}
	//passParams = {};
	mLightVector.clear();
	mObjectVector.clear();
	mDummyVector.clear();

}



void RenderSystem::SetTable()
{
	mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

	GRAPHICS_CMD_LIST->SetGraphicsRootSignature(mRootSignature->GetRootSignature().Get());	// 루트시그니쳐 set

	ID3D12DescriptorHeap* descHeap = gEngine->GetRenderManager().GetLegacyGraphicsDescriptorHeap();
	GRAPHICS_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	//몇번째 테이블힙을 사용할건지 선택함 (매우 무거움으로 프레임당 1번만 사용할것을 권장함)



	// Table 바인딩
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount, 1, GBUFFER_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount, 2, GROUP_START, GROUP_COUNT);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount, 3, PARTICLE_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount, 4, TEXTURE_MATERIALS_INDEX_START);

}

void RenderSystem::PushPassData()
{
	
	passParams.MatView = mCamera->mView;
	passParams.MatProjection = mCamera->mProjection;
	passParams.MatViewInv = mCamera->mView.Invert();
	passParams.MatProjectionInv = mCamera->mProjection.Invert();
	passParams.ScreenSize = { static_cast<float>(RENDERMANAGER.GetWindow().Width), static_cast<float>(RENDERMANAGER.GetWindow().Height) };


	shared_ptr<GroupBuffer> a = RENDERMANAGER.GetGroupBuffer(mFrameCount);
	a->PassInfo->PushData(&passParams, sizeof(PassParams));
	a->PassInfo->PushData(&passParams, sizeof(PassParams));
}


void RenderSystem::PushGBufferData()
{

}

void RenderSystem::PushMaterialData()
{

	uint32 index{};
	for (auto& materials : RESOURCEMANAGER.GetAllResources<Material>()) {
		shared_ptr<Material> material=dynamic_pointer_cast<Material>(materials.second);
		material->SetIndex(index);
		mMaterialVector.emplace_back(material->GetParams());
		index++;
	}
	RENDERMANAGER.GetMaterialBuffers()->PushDefaultToData(mMaterialVector.data(), static_cast<uint32>( mMaterialVector.size() * sizeof(MaterialParams)));

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

		lightParams.MatWorld = transformComponent->mWorldMatrix;
		lightParams.MatView = cameraComponent->mCameraParams.MatView;
		lightParams.MatProjection = cameraComponent->mCameraParams.MatProjection;
		lightParams.MatViewInv = cameraComponent->mCameraParams.MatViewInv;
		lightParams.MatProjectionInv = cameraComponent->mCameraParams.MatProjectionInv;

		mLightVector.push_back(lightParams);

	}		
	RENDERMANAGER.GetGroupBuffer(mFrameCount)->LightInfo->PushGraphicsData(mLightVector.data(), static_cast<uint32>(sizeof(LightParams)*mLightVector.size()));
	
}

void RenderSystem::PushObjectData()
{
	TransformComponent* transformComponent;
	RenderComponent* renderComponent;
	const vector<EntityID>& gameObjects = mRenderComponentPool->GetEntities();

	for (const EntityID& gameObject : gameObjects)		// 같은 머테리얼을 가진 것끼리 분류
	{
		if (mWorld->GetComponent<LightComponent>(gameObject)) {
			continue;
		}

		const uint64 instanceId = mWorld->GetComponent<RenderComponent>(gameObject)->GetInstanceID();
		mMaterialObjectBatch[instanceId].push_back(gameObject);
	}

	uint32 index{};
	for (auto& materialObjects : mMaterialObjectBatch){

		for (auto& objects : materialObjects.second) {

			transformComponent = mWorld->GetComponent<TransformComponent>(objects);;
			renderComponent = mWorld->GetComponent<RenderComponent>(objects);;


			objectParams.MatWorld = transformComponent->mWorldMatrix;
			mObjectVector.push_back(objectParams);		// 트랜스폼 갱신
			
			renderComponent->mObjectIndex = index;
			index++;
		}

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


	for (auto& entityID : mRenderComponentPool->GetEntities()) {
		RenderComponent* renderEntity = mRenderComponentPool->GetComponent(entityID);
		if (renderEntity->IsVisibility())
			continue;
		if (mWorld->GetComponent<LightComponent>(entityID)) {
			continue;
		}
	/*	if (IsCustomCulled(renderEntity->GetLayerIndex()))
			continue;*/
			
		if (IsFrustumCulled()) {
			if (mCamera->mFrustum.ContainsSphere(
				mWorld->GetComponent<TransformComponent>(entityID)->GetWorldPosition(),
				mWorld->GetComponent<TransformComponent>(entityID)->GetBoundingSphereRadius()) == false)
			{
				continue;
			}
		}
			
		shaderBatches[static_cast<uint8>(SHADER_TYPE::DEFERRED)][renderEntity->mMaterials[0]->GetShaderID()].push_back(entityID);

	}


	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::DEFERRED)]) {
		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		InstancingRender(vec);
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

		RESOURCEMANAGER.Get<Shader>(lightComponent->mMaterials[0]->GetShaderID())->Update();


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
	RESOURCEMANAGER.Get<Material>(L"Final");
	
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
}

void RenderSystem::RenderForward()
{
	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::FORWARD)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		InstancingRender(vec);

	}
	
	//나머지는 바로 forward

	//for (auto& camera : _cameras)
	//{
	//	if (camera == mainCamera)
	//		continue;

	//	camera->SortGameObject();
	//	camera->Render_Forward();
	//}
}

void RenderSystem::RenderingParticle()
{

	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::PARTICLE)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();


		for (auto& particle : vec)
		{
			ParticleComponent* particleComponent = mWorld->GetComponent<ParticleComponent>(particle);
			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(particle);



			particleComponent->_mesh->Render(particleComponent->_maxParticle);
		}
	}

}

bool RenderSystem::IsFrustumCulled()
{
	

	return false;
}

void RenderSystem::RenderShadowCamera(Entity& light , LightComponent* lightComponent, CameraComponent* cameraComponent, RenderComponent* renderComponent)
{
	TransformComponent* transformComponent;
	RenderComponent* objectRenderComponent;
	
	
	RESOURCEMANAGER.Get<Shader>(L"Shadow")->Update();

	for ( const auto& gameObjects : mMaterialObjectBatch)
	{
		if (gameObjects.second.empty()) {
			continue;
		}

		for (const auto& gameObject : gameObjects.second) {

			

			transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
			objectRenderComponent = mWorld->GetComponent<RenderComponent>(gameObject);


			//if (gameObject->IsStatic())	//정적 물체인지 동적물체인지 확인해서 그림자 최적화
			//	continue;


			//if (IsCustomCulled(renderComponent->GetLayerIndex()))
			//	continue;

			if (objectRenderComponent->mCheckFrustum)
			{
				if (cameraComponent->mFrustum.ContainsSphere(
					transformComponent->GetWorldPosition(),
					transformComponent->GetBoundingSphereRadius()) == false)
				{
					continue;
				}

			}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        

			mDummyVector.push_back(gameObject);

		}

		if (mDummyVector.empty()) {
			continue;
		}

		InstancingRender(mDummyVector);

	}

}

void RenderSystem::InstancingRender(vector<Entity>& gameObjects)
{
	Entity entity0 = gameObjects[0];

	RenderComponent* objectRender;
	wstring	shaderID{L""};

	if (gameObjects.size() == 1)
	{
			
		objectRender = mRenderComponentPool->GetComponent(entity0.GetID());

		for (int i = 0; i < objectRender->mMaterials.size(); ++i) {
			

			renderParams = { objectRender->mObjectIndex ,objectRender->mMaterials[i]->GetIndex()};

			if (shaderID != objectRender->mMaterials[i]->GetShaderID()) {
				shaderID = objectRender->mMaterials[i]->GetShaderID();
				RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
			}

			GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 4, &(renderParams), 0);
			objectRender->mMesh->Render(1, i);
		}
	}
	else
	{	
		for (auto& entity : gameObjects) {
			objectRender = mRenderComponentPool->GetComponent(entity.GetID());
			for (int i = 0; i < objectRender->mMaterials.size(); ++i) {


				renderParams = { objectRender->mObjectIndex ,objectRender->mMaterials[i]->GetIndex() };
			
				if (shaderID != objectRender->mMaterials[i]->GetShaderID()) {
					shaderID = objectRender->mMaterials[i]->GetShaderID();
					RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
				}

				
				GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 4, &(renderParams), 0);

				objectRender->mMesh->Render(static_cast<uint32>(gameObjects.size()), i);
			}
		}
	}
	
}


//
//void RenderSystem::Render(Entity entity)
//{
//	RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
//	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
//	//AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);
//
//	for (uint32 i = 0; i < renderComponent->mMaterials.size(); i++)
//	{
//		shared_ptr<Material>& material = renderComponent->mMaterials[i];
//
//		if (material == nullptr || material->GetShader() == nullptr)
//			continue;
//
//		//if (animationComponent)
//		//{
//		//	//GetAnimator()->PushData();
//		//	material->SetInt(1, 1);
//		//}
//
//		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &renderComponent->mdataIndex, 0);
//		renderComponent->mMesh->Render(1, i);
//	}
//}
//
//void RenderSystem::Render(Entity entity,shared_ptr<InstancingBuffer>& buffer)
//{
//	RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
//	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
//	AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);
//
//	for (uint32 i = 0; i < renderComponent->mMaterials.size(); i++)
//	{
//		shared_ptr<Material>& material = renderComponent->mMaterials[i];
//
//		if (material == nullptr || material->GetShader() == nullptr)
//			continue;
//
//		buffer->PushData();
//
//		if (animationComponent)
//		{
//			//animationComponent->PushData();
//			material->SetInt(1, 1);
//		}
//
//		material->PushGraphicsData();
//		renderComponent->mMesh->Render(buffer, i);
//	}
//}


// 의사코드
// 1. 모든 랜더컴포넌트 보유 오브젝트 프러스텀 컬링
//	1- 2.	컬링 결과가 TRUE면 : VISIBLE TRUE
//				SETSTRUCTUEDBUFFER_GPU리소스버퍼에 값 갱신
//				memcpy(&visibleGpuData[visibleCount++], &allGpuData[i], sizeof(GPUData));
// 
//			컬링 결과가 FALSE면 : VISIBLE FALSE
//	
// 2. SETSTRUCTUEDBUFFER(); 
// 3. VISBLE값이 TRUE인 OBJECT만
//	3-2. 인덱스값을 CB에 넣어서 모든 오브젝트 랜더링 시작

