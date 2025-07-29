#include "pch.h"
#include "RenderSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "World.h"
#include "RenderComponent.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
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
	GRAPHICS_CMD_LIST->SetGraphicsRootSignature(mRootSignature->GetRootSignature().Get());


	if (1) {	// Find Main Camera.
		std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
		mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);

	}

	PushLightData();

	ClearRTV();

	RenderShadow();

	RenderDeferred();

	RenderLights();

	RenderFinal();	//2pass

	//RenderForward();

}

void RenderSystem::PushLightData()
{
	LightParams lightParams = {};

	ComponentPool<LightComponent>& lightComponents = mWorld->GetComponentPool<LightComponent>();

	for (auto& light : lightComponents)
	{
		

		light.SetLightIndex(lightParams.lightCount);	//자기가 몇번째 light인지 확인

		lightParams.lights[lightParams.lightCount] = light.mLightInfo;
		lightParams.lightCount++;
	}

	CONST_BUFFER(CONSTANT_BUFFER_TYPE::GLOBAL)->SetGraphicsGlobalData(&lightParams, sizeof(lightParams));
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

}

void RenderSystem::RenderShadow()
{

	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->OMSetRenderTargets();

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

	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->WaitTargetToResource();
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
			if (mCamera->_frustum.ContainsSphere(
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
	mWorld->GetEntitiesWithComponents<LightComponent, TransformComponent, CameraComponent>();

	for(auto& light : lights)
	{
		
		LightComponent* lightComponent =  mWorld->GetComponent<LightComponent>(light);

		assert(lightComponent->_lightIndex >= 0);

		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(light);
		PushTransformData(transformComponent);

		if (static_cast<LIGHT_TYPE>(lightComponent->mLightInfo.LightType) == LIGHT_TYPE::DIRECTIONAL_LIGHT)
		{
			shared_ptr<Texture> shadowTex =RESOURCEMANAGER.Get<Texture>(L"ShadowTarget");
			lightComponent->_lightMaterial->SetTexture(2, shadowTex);

			CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(light);

			Matrix matVP = cameraComponent->GetViewMatrix() * cameraComponent->GetProjectionMatrix();
			lightComponent->_lightMaterial->SetMatrix(0, matVP);
		}
		else
		{
			float scale = 2 * lightComponent->mLightInfo.range;
			transformComponent ->SetLocalScale(Vec3(scale, scale, scale));
		}

		lightComponent->_lightMaterial->SetInt(0, lightComponent->_lightIndex);
		lightComponent->_lightMaterial->PushGraphicsData();

		//switch (static_cast<LIGHT_TYPE>(_lightInfo.lightType))
		//{
		//case LIGHT_TYPE::POINT_LIGHT:
		//case LIGHT_TYPE::SPOT_LIGHT:
		//	float scale = 2 * _lightInfo.range;
		//	GetTransform()->SetLocalScale(Vec3(scale, scale, scale));	//빛의 영역을 설정
		//	break;
		//}

		lightComponent->_volumeMesh->Render();
	}



	////리소스에서 타켓으로
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->WaitTargetToResource();
}

void RenderSystem::RenderFinal()
{
	// Swapchain OMSet
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->OMSetRenderTargets(1, backIndex);

	
	RESOURCEMANAGER.Get<Material>(L"Final")->PushGraphicsData();
	RESOURCEMANAGER.Get<Shader>(L"Final")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
}

void RenderSystem::RenderForward()
{
	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::FORWARD)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();
		InstancingRender(vec);
	}
	
	for (auto& [shaderID, vec] : shaderBatches[static_cast<uint8>(SHADER_TYPE::PARTICLE)]) {

		RESOURCEMANAGER.Get<Shader>(shaderID)->Update();


		for (auto& particle : vec)
		{
			ParticleComponent* particleComponent = mWorld->GetComponent<ParticleComponent>(particle);

			PushTransformData(mWorld->GetComponent<TransformComponent>(particle));

			particleComponent->_particleBuffer->PushGraphicsData(SRV_REGISTER::t9);
			particleComponent->_material->SetFloat(0, particleComponent->_startScale);
			particleComponent->_material->SetFloat(1, particleComponent->_endScale);
			particleComponent->_material->PushGraphicsData();

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

	const vector<EntityID>& gameObjects = mRenderComponentPool->GetEntities();
	for (const EntityID& gameObject : gameObjects)
	{
		
		transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);;
		renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);;
		

		//if (gameObject->IsStatic())	//정적 물체인지 동적물체인지 확인해서 그림자 최적화
		//	continue;


		//if (IsCustomCulled(renderComponent->GetLayerIndex()))
		//	continue;

		if (renderComponent->_checkFrustum)
		{
			if (cameraComponent->_frustum.ContainsSphere(
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
			

			transformComponent = mWorld->GetComponent<TransformComponent>(shadow);;
			renderComponent = mWorld->GetComponent<RenderComponent>(shadow);;

			PushTransformData(transformComponent);

			RESOURCEMANAGER.Get<Material>(L"Shadow")->PushGraphicsData();
			renderComponent->mMesh->Render();
		}
	}
}

void RenderSystem::InstancingRender(vector<Entity>& gameObjects)
{


	

	for (Entity& gameObject : gameObjects)
	{
		const uint64 instanceId = mWorld->GetComponent<RenderComponent>(gameObject)->GetInstanceID();
		cache[instanceId].push_back(gameObject);
	}

	

	for (auto& pair : cache)	// 같은 머테리얼을 가진것 끼리 분류
	{
		Entity entity0 = pair.second[0];

		
		ComponentPool<TransformComponent>& tobject = mWorld->GetComponentPool<TransformComponent>();

		RenderComponent* object = mRenderComponentPool->GetComponent(entity0.GetID());
		if (pair.second.size() == 1)
		{
			tobject.GetComponent(entity0.GetID())->FinalUpdate();
			Render(entity0);
			
		}
		else
		{
			const uint64 instanceId = pair.first;
			

			for (const Entity& gameObject : pair.second)
			{
				tobject.GetComponent(gameObject.GetID())->FinalUpdate();
				InstancingParams params;
				params.matWorld = tobject.GetComponent(gameObject.GetID())->GetLocalToWorldMatrix();
				params.matWV = params.matWorld * mCamera->_matView;
				params.matWVP = params.matWorld * mCamera->_matView * mCamera->_matProjection;

				AddParam(instanceId, params);
			}

			shared_ptr<InstancingBuffer>& buffer = _buffers[instanceId];
			Render(entity0, buffer);
		}
	}
}

void RenderSystem::AddParam(uint64 instanceId, InstancingParams& data)
{

		if (_buffers.find(instanceId) == _buffers.end())
			_buffers[instanceId] = make_shared<InstancingBuffer>();


		_buffers[instanceId]->AddData(data);
	
}

void RenderSystem::PushTransformData(TransformComponent* transformComponent)
{
	TransformParams transformParams = {};
	transformParams.matWorld = transformComponent->_matWorld;
	transformParams.matView = mCamera->_matView;
	transformParams.matProjection = mCamera->_matProjection;
	transformParams.matWV = transformComponent->_matWorld * mCamera->_matView;
	transformParams.matWVP = transformComponent->_matWorld * mCamera->_matView * mCamera->_matProjection;
	transformParams.matViewInv = mCamera->_matView.Invert();

	CONST_BUFFER(CONSTANT_BUFFER_TYPE::TRANSFORM)->PushGraphicsData(&transformParams, sizeof(transformParams));
}

void RenderSystem::ClearBuffer()
{
	for (auto& pair : _buffers)
	{
		shared_ptr<InstancingBuffer>& buffer = pair.second;
		buffer->Clear();
	}

	for (auto& shaderGroup : shaderBatches) {

		for (auto& [shaderID, vec] : shaderGroup) {
			vec.clear(); // vector의 capacity는 유지됨
		}
	}

	 for (auto& [ID, vec] : cache) {
	 	vec.clear();
	 }

}


void RenderSystem::Render(Entity entity)
{
	RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
	//AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);

	for (uint32 i = 0; i < renderComponent->mMaterials.size(); i++)
	{
		shared_ptr<Material>& material = renderComponent->mMaterials[i];

		if (material == nullptr || material->GetShader() == nullptr)
			continue;

		PushTransformData(transformComponent);

		//if (animationComponent)
		//{
		//	//GetAnimator()->PushData();
		//	material->SetInt(1, 1);
		//}

		material->PushGraphicsData();
		renderComponent->mMesh->Render(1, i);
	}
}

void RenderSystem::Render(Entity entity,shared_ptr<InstancingBuffer>& buffer)
{
	RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
	AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);

	for (uint32 i = 0; i < renderComponent->mMaterials.size(); i++)
	{
		shared_ptr<Material>& material = renderComponent->mMaterials[i];

		if (material == nullptr || material->GetShader() == nullptr)
			continue;

		buffer->PushData();

		if (animationComponent)
		{
			//animationComponent->PushData();
			material->SetInt(1, 1);
		}

		material->PushGraphicsData();
		renderComponent->mMesh->Render(buffer, i);
	}
}


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

