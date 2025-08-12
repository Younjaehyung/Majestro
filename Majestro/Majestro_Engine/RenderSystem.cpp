#include "pch.h"
#include "RenderSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "World.h"
#include "RenderComponent.h"
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
	
}

void RenderSystem::Update()
{
	mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

	GRAPHICS_CMD_LIST->SetGraphicsRootSignature(mRootSignature->GetRootSignature().Get());	// 루트시그니쳐 set

	// Table 바인딩
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount,0,GBUFFER_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount,1,GROUP_START,GROUP_COUNT);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount,2,PARTICLE_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitTable(mFrameCount,3,TEXTURE_INDEX_START);



	if (1) {	// Find Main Camera.
		std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
		mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);
	}

	ClearRTV();

	PushLightData();

	PushObjectData();
	// RenderShadow();

	RenderDeferred();

	RenderLights();

	RenderFinal();	//2pass

	RenderForward();

}

void RenderSystem::ClearRTV()
{
	//CommandQueue의 RT이 이리로 옮겨짐
	// SwapChain Group 초기화
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->ClearRenderTargetView(backIndex);

	// Shadow Group 초기화
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->ClearRenderTargetView();

	// Deferred Group 초기화
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->ClearRenderTargetView();

	// Lighting Group 초기화
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->ClearRenderTargetView();

	ClearBuffer();
	// 추후 lightvector관련들 clear도 모두 확인할껏.
}


void RenderSystem::PushLightData()
{
	LightParams lightParams = {};
	MaterialParams materialParams = {};

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

		if (static_cast<LIGHT_TYPE>(lightComponent->mLightInfo.LightType) == LIGHT_TYPE::DIRECTIONAL_LIGHT)
		{
			shared_ptr<Texture> shadowTex = RESOURCEMANAGER.Get<Texture>(L"ShadowTarget");
			renderComponent->mMaterials[0]->SetTexture(shadowTex, DIFFUSEMAP2INDEX);
		}
		else
		{
			float scale = 2 * lightComponent->mLightInfo.Range;
			transformComponent->SetLocalScale(Vec3(scale, scale, scale));
		}
		lightParams = {};

		lightParams.Color = lightComponent->mLightInfo.Color;
		lightParams.Position = lightComponent->mLightInfo.Position;
		lightParams.Direction = lightComponent->mLightInfo.Direction;
		lightParams.LightType = lightComponent->mLightInfo.LightType;
		lightParams.Range = lightComponent->mLightInfo.Range;
		lightParams.Angle = lightComponent->mLightInfo.Angle;

		lightParams.MatWorld = transformComponent->mWorldMatrix;
		lightParams.MatView = cameraComponent->mCameraParams.MatProjection;
		lightParams.MatProjection = cameraComponent->mCameraParams.MatProjection;
		lightParams.MatViewInv = cameraComponent->mCameraParams.MatViewInv;
		lightParams.MatProjectionInv = cameraComponent->mCameraParams.MatProjectionInv;

		mLightVector.push_back(lightParams);

	}		
	RENDERMANAGER.GetGroupBuffer()[mFrameCount].LightInfo->PushGraphicsData(mLightVector.data(),sizeof(LightParams)*mLightVector.size());

	int MaterialsIndex;
}

void RenderSystem::PushObjectData()
{
	TransformComponent* transformComponent;
	RenderComponent* renderComponent;
	const vector<EntityID>& gameObjects = mRenderComponentPool->GetEntities();

	DataIndex dataParam{};

	for (const EntityID& gameObject : gameObjects)		// 같은 머테리얼을 가진 것끼리 분류
	{
		const uint64 instanceId = mWorld->GetComponent<RenderComponent>(gameObject)->GetInstanceID();
		mMaterialObjectBatch[instanceId].push_back(gameObject);
	}


	for (auto& pair : mMaterialObjectBatch)		// renderComponent내부에 현재 몇번째 인덱스를 참조해야하는지 표기.
	{
		Entity entity0 = pair.second[0];
		uint32 matrialIndexStart = static_cast<uint32>(mMaterialVector.size());	// 머티리얼 시작인덱스

		if (pair.second.size() == 1)
		{
			transformComponent = mWorld->GetComponent<TransformComponent>(entity0);;
			renderComponent = mWorld->GetComponent<RenderComponent>(entity0);;

			PushObjectData(transformComponent);		// 트랜스폼 갱신
			PushMaterialData(renderComponent);			// 머티리얼 갱신

		}
		else
		{
			const uint64 instanceId = pair.first;
			renderComponent = mWorld->GetComponent<RenderComponent>(pair.second[0]);;
			PushMaterialData(renderComponent);		// 머티리얼 갱신

			for (const Entity& gameObject : pair.second)
			{
				transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);;
				PushObjectData(transformComponent);		// 트랜스폼 갱신

			}

		}

	}


	RENDERMANAGER.GetGroupBuffer()[mFrameCount].ObjectInfo->PushGraphicsData(mObjectVector.data(),sizeof(objectParams)*mObjectVector.size());
	RENDERMANAGER.GetGroupBuffer()[mFrameCount].ObjectInfo->PushGraphicsData(mMaterialVector.data(), sizeof(MaterialParams) * mMaterialVector.size());

}




void RenderSystem::RenderShadow()
{

	RENDERMANAGER.GetRTGroup()->OMSetRenderTargets(2);

	LightComponent* lightComponent;
	CameraComponent* cameraComponent;

	for (auto& light : mWorld->GetEntitiesWithComponents<LightComponent,CameraComponent>())
	{
		lightComponent = mWorld->GetComponent<LightComponent>(light);
		cameraComponent = mWorld->GetComponent<CameraComponent>(light);
		if (lightComponent->mLightInfo.LightType != static_cast<int32>(LIGHT_TYPE::DIRECTIONAL_LIGHT))
			continue;

		RenderShadowCamera(light, lightComponent, cameraComponent);
	}


	RENDERMANAGER.GetRTGroup()->WaitTargetToResource();
}

void RenderSystem::RenderDeferred()
{
	std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);


	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->OMSetRenderTargets();


	for (auto& entityID : mRenderComponentPool->GetEntities()) {
		RenderComponent* renderEntity = mRenderComponentPool->GetComponent(entityID);
		if (renderEntity->IsVisibility())
			continue;

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
			
		uint8 typeID = static_cast<uint8>(renderEntity->mMaterials[0]->GetShader()->GetShaderType());

		shaderBatches[typeID][renderEntity->mMaterials[0]->GetShaderID()].push_back(entityID);

		//인스턴싱 구조 생각하기
	}

		
	

	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::DEFERRED)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		InstancingRender(vec);
	}
	

	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->WaitTargetToResource();
}

void RenderSystem::RenderLights()
{

	//Camera::S_MatView = mCamera->GetViewMatrix();
	//Camera::S_MatProjection = mCamera->GetProjectionMatrix();

	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->OMSetRenderTargets();

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


		lightComponent->mMesh->Render();
	}


	////리소스에서 타켓으로
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->WaitTargetToResource();
}

void RenderSystem::RenderFinal()
{
	// Swapchain OMSet
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->OMSetRenderTargets(1, backIndex);

	
	RESOURCEMANAGER.Get<Shader>(L"Final")->Update();
	RESOURCEMANAGER.Get<Material>(L"Final");
	
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
}

void RenderSystem::RenderForward()
{
	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::FORWARD)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		for (auto& v : vec) {
			RenderComponent* r = mWorld->GetComponent<RenderComponent>(v);

			GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0,3,&(r->mdataIndex),0);

			for (int i = 0; i < r->mMaterials.size(); ++i) {
				r->mMesh->Render(1,i);
			}
			
		}

	}
	
	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::PARTICLE)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();


		for (auto& particle : vec)
		{
			ParticleComponent* particleComponent = mWorld->GetComponent<ParticleComponent>(particle);
			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(particle);

	

			particleComponent->_mesh->Render(particleComponent->_maxParticle);
		}
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

bool RenderSystem::IsFrustumCulled()
{
	

	return false;
}

void RenderSystem::RenderShadowCamera(Entity& light , LightComponent* lightComponent, CameraComponent* cameraComponent)
{
	TransformComponent* transformComponent;
	RenderComponent* renderComponent;
	
	// 추후 CBV 중복 삽입 문제 있나 확인해보기
	const vector<EntityID>& gameObjects = mRenderComponentPool->GetEntities();
	for (const EntityID& gameObject : gameObjects)
	{
		
		transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);;
		renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);;
		

		//if (gameObject->IsStatic())	//정적 물체인지 동적물체인지 확인해서 그림자 최적화
		//	continue;


		//if (IsCustomCulled(renderComponent->GetLayerIndex()))
		//	continue;

		if (renderComponent->mCheckFrustum)
		{
			if (cameraComponent->mFrustum.ContainsSphere(
				transformComponent->GetWorldPosition(),
				transformComponent->GetBoundingSphereRadius()) == false)
			{
				continue;
			}
		}
		
		shaderBatches[static_cast<uint8>(SHADER_TYPE::SHADOW)][renderComponent->mMaterials[0]->GetShaderID()].push_back(gameObject);


	}

	mCamera = cameraComponent;

	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::SHADOW)])
	{
		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		for (auto& shadow : vec) {

			renderComponent = mWorld->GetComponent<RenderComponent>(shadow);
			for (int i = 0; i < renderComponent->mMaterials.size(); ++i) {

				GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0,3, &renderComponent->mdataIndex,0);
				renderComponent->mMesh->Render(1,i);
			}
			
		}
	}
}

void RenderSystem::InstancingRender(vector<Entity>& gameObjects)
{


	for (auto& pair : mMaterialObjectBatch)	// 같은 머테리얼을 가진것 끼리 분류
	{
		Entity entity0 = pair.second[0];

		RenderComponent* object = mRenderComponentPool->GetComponent(entity0.GetID());
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &(object->mdataIndex), 0);

		if (pair.second.size() == 1)
		{
			
			for (int i = 0; i < object->mMaterials.size(); ++i) {
				object->mMesh->Render(1, i);
			}
		}
		else
		{
			const uint64 instanceId = pair.first;

			for (int i = 0; i < object->mMaterials.size(); ++i) {
				object->mMesh->Render(static_cast<uint32>(pair.second.size()), i);
			}
			
		}
	}
}

void RenderSystem::PushGBufferData()
{
	RESOURCEMANAGER.Get<Texture>(L"SpecularLightTarget")->get;
	GRAPHICS_CMD_LIST->des
}

void RenderSystem::PushPassData()
{
	PassParams passParams{};
	passParams.MatView = mCamera->mView;
	passParams.MatProjection = mCamera->mProjection;
	passParams.MatViewInv = mCamera->mView.Invert();
	passParams.MatProjectionInv = mCamera->mProjection.Invert();

	passParams.ScreenSize = { static_cast<float>(RENDERMANAGER.GetWindow().Width), static_cast<float>(RENDERMANAGER.GetWindow().Height) };
	
	
	RENDERMANAGER.GetGroupBuffer()[mFrameCount].PassInfo->PushData(&passParams,sizeof(PassParams));
}

void RenderSystem::PushObjectData(TransformComponent* transformComponent)
{
	
	objectParams.MatWorld = transformComponent->mWorldMatrix;

	mObjectVector.push_back(objectParams);
}

void RenderSystem::PushMaterialData(RenderComponent* renderComponent)
{
	for (auto& material : renderComponent->mMaterials) {
		mMaterialVector.push_back(material->GetParams());
	}

	
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

	 mLightVector.clear();
	 mObjectVector.clear();
	 mMaterialVector.clear();
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

