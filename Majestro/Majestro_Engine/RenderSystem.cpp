#include "pch.h"
#include "RenderSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "World.h"
#include "RenderComponent.h"

#include "TagComponent.h"


void RenderSystem::Initialize()
{
	// 1번 초기화
	mRenderComponentPool = &(mWorld->GetComponentPool<RenderComponent>());
}

void RenderSystem::Update()
{


	mWorld->GetComponentPool<RenderComponent>();

	PushLightData();

	ClearRTV();

	RenderShadow();

	RenderDeferred();

	RenderLights();


	RenderFinal();	//2pass

	RenderForward();

}

void RenderSystem::PushLightData()
{
	//LightParams lightParams = {};

	//for (auto& light : _lights)
	//{
	//	const LightInfo& lightInfo = light->GetLightInfo();

	//	light->SetLightIndex(lightParams.lightCount);	//자기가 몇번째 light인지 확인

	//	lightParams.lights[lightParams.lightCount] = lightInfo;
	//	lightParams.lightCount++;
	//}

	//CONST_BUFFER(CONSTANT_BUFFER_TYPE::GLOBAL)->SetGraphicsGlobalData(&lightParams, sizeof(lightParams));
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


}

void RenderSystem::RenderShadow()
{
	//RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->OMSetRenderTargets();

	//for (auto& light : mWorld->GetEntitiesWithComponent<RenderComponent>())
	//{
	//	if (light->GetLightType() != LIGHT_TYPE::DIRECTIONAL_LIGHT)
	//		continue;

	//	light->RenderShadow();
	//}

	//RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->WaitTargetToResource();
}

void RenderSystem::RenderDeferred()
{
//	// Deferred OMSet
//	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->OMSetRenderTargets();
//
//	shared_ptr<Camera> mainCamera = _cameras[0];	//처음 추가된 카메라를 메인카메라로 임시 설정함
//	mainCamera->SortGameObject();
//	mainCamera->Render_Deferred();	//1pass
//
//	//리소스용도와 출력용도를 나누기 위해 
//	//타켓에서 리소스로
//	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->WaitTargetToResource();
//
	
	std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	// Find Main Camera.


	if (camera.empty()) {
		return;
	}


	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->OMSetRenderTargets();

	// 1. 쉐이더 배치 처리


	auto& shaderMap = RESOURCEMANAGER.GetAllResources<Shader>();

	for (auto& [key, object] : shaderMap)
	{
		shared_ptr<Shader> shader = static_pointer_cast<Shader>(object);
		// 이제 shader로 원하는 작업 수행 가능

		shader->Update();

		for (int i = 0; i < mRenderComponentPool->Size(); ++i) {

			if (IsCulled(mRenderComponentPool->GetComponent(i)->GetLayerIndex()))
				continue;
			if(IsFrustumCulled( ))


			//인스턴싱 구조 생각하기
		}
		mRenderComponentPool->GetComponent(i)->mMaterial->PushGraphicsData();
		mRenderComponentPool->GetComponent(i)->mMesh->Render();

		
	}



	_vecForward.clear();
	_vecDeferred.clear();
	_vecParticle.clear();



	for (auto& gameObject : gameObjects)
	{
		if (gameObject->GetMeshRenderer() == nullptr && gameObject->GetParticleSystem() == nullptr)
			continue;

		if (IsCulled(gameObject->GetLayerIndex()))
			continue;

		if (gameObject->GetCheckFrustum())
		{
			if (_frustum.ContainsSphere(
				gameObject->GetTransform()->GetWorldPosition(),
				gameObject->GetTransform()->GetBoundingSphereRadius()) == false)
			{
				continue;
			}
		}

		if (gameObject->GetMeshRenderer())
		{
			SHADER_TYPE shaderType = gameObject->GetMeshRenderer()->GetMaterial()->GetShader()->GetShaderType();
			switch (shaderType)
			{
			case SHADER_TYPE::DEFERRED:
				_vecDeferred.push_back(gameObject);
				break;
			case SHADER_TYPE::FORWARD:
				_vecForward.push_back(gameObject);
				break;
			}
		}
		else
		{
			_vecParticle.push_back(gameObject);
		}
	}



	S_MatView = _matView;
	S_MatProjection = _matProjection;

	GET_SINGLE(InstancingManager)->Render(_vecDeferred);


	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->WaitTargetToResource();
}

void RenderSystem::RenderLights()
{
	//shared_ptr<Camera> mainCamera = _cameras[0];
	//Camera::S_MatView = mainCamera->GetViewMatrix();
	//Camera::S_MatProjection = mainCamera->GetProjectionMatrix();

	//GEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->OMSetRenderTargets();

	//// 광원을 그린다.
	//// 광원을 기준으로 나머지 객체들을 그린다

	//for (auto& light : _lights)
	//{
	//	light->Render();
	//}

	////리소스에서 타켓으로
	//GEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->WaitTargetToResource();
}

void RenderSystem::RenderFinal()
{
	// Swapchain OMSet
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->OMSetRenderTargets(1, backIndex);

	//RESOURCEMANAGER.Get<Material>(L"Final")->PushGraphicsData();
	//RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
}

void RenderSystem::RenderForward()
{
	//shared_ptr<Camera> mainCamera = _cameras[0];
	//mainCamera->Render_Forward();	//메인 카메라는 Deferred후 forward 


	////나머지는 바로 forward

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

void RenderSystem::IsCulled()
bool IsCulled(uint8 layer) { return (_cullingMask & (1 << layer)) != 0; }

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

