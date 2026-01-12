#include "pch.h"
#include "EffectSystem.h"

#include "World.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"

#include "TagComponent.h"
#include "CameraComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "VfxComponent.h"
#include "Vfx.h"

EffectSystem::EffectSystem(World* world) : System::System(world)
{
}

EffectSystem::~EffectSystem()
{

	
	Shutdown();
	//CoUninitialize();
	
}

void EffectSystem::Shutdown()
{
	auto resources = mWorld->GetEntitiesWithComponent<VfxComponent>();
	for (auto& res : resources)
	{
		VfxComponent* effect = mWorld->GetComponent<VfxComponent>(res);
		if (effect && effect->efkHandle != -1)
		{
			manager_->StopEffect(effect->efkHandle);
		}
	}


	commandList_.Reset();
	memoryPool_.Reset();
	//

	manager_.Reset();
	setting_.Reset();

	renderer_.Reset();
	graphicsDevice_.Reset();


	// RefPtr 기반이면 보통 nullptr 대입으로 정리됩니다.
	//commandList_ = nullptr;
	//memoryPool_ = nullptr;
	//    manager_ = nullptr;
	//    renderer_ = nullptr;
	/*graphicsDevice_ = nullptr;*/
	//   setting_ = nullptr;
	
}

void EffectSystem::Initialize()
{	
	Initialize(
		DEVICE.Get(),
		RENDERMANAGER.GetGraphicsCmdQueue()->GetCommandQueue().Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		SWAP_CHAIN_BUFFER_COUNT,
		false,
		2000,
		2000);

	LoadResources();
}

bool EffectSystem::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, int32_t swapBufferCount, bool isReversedDepth, int32_t instanceMax, int32_t squareMaxCount)
{

	graphicsDevice_ = EffekseerRendererDX12::CreateGraphicsDevice(device, commandQueue, swapBufferCount);
	if (graphicsDevice_ == nullptr) return false; // CreateGraphicsDevice :contentReference[oaicite:8]{index=8}

	DXGI_FORMAT rtFormats[1] = { rtvFormat };
	renderer_ = EffekseerRendererDX12::Create(
		graphicsDevice_,
		rtFormats,
		1,
		dsvFormat,
		isReversedDepth,
		squareMaxCount);
	if (renderer_ == nullptr) return false; // Create :contentReference[oaicite:9]{index=9}

	manager_ = Effekseer::Manager::Create(instanceMax);
	if (manager_ == nullptr) return false;

	setting_ = Effekseer::Setting::Create();
	setting_->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	memoryPool_ = EffekseerRenderer::CreateSingleFrameMemoryPool(graphicsDevice_);
	commandList_ = EffekseerRenderer::CreateCommandList(graphicsDevice_, memoryPool_);


	manager_->SetSpriteRenderer(renderer_->CreateSpriteRenderer());
	manager_->SetRibbonRenderer(renderer_->CreateRibbonRenderer());
	manager_->SetRingRenderer(renderer_->CreateRingRenderer());
	manager_->SetTrackRenderer(renderer_->CreateTrackRenderer());
	manager_->SetModelRenderer(renderer_->CreateModelRenderer());

	manager_->SetTextureLoader(renderer_->CreateTextureLoader());
	manager_->SetModelLoader(renderer_->CreateModelLoader());
	manager_->SetMaterialLoader(renderer_->CreateMaterialLoader());
	manager_->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());




	return (memoryPool_ != nullptr && commandList_ != nullptr);
}


Effekseer::EffectRef EffectSystem::LoadEffect(const EFK_CHAR * path, float magnification,const EFK_CHAR * materialPath )
{
	// Effect::Create(manager, path, magnification, materialPath) :contentReference[oaicite:14]{index=14}
	return Effekseer::Effect::Create(manager_, path, magnification, materialPath);
}

Effekseer::EffectRef EffectSystem::LoadEffect(const std::string_view path, float magnification, const std::string_view materialPath)
{
	EfkString efkPath = ToEfkString(path);
	EfkString efkMat = materialPath.empty() ? EfkString{} : ToEfkString(materialPath);

	const EFK_CHAR* matPtr = materialPath.empty() ? nullptr : efkMat.c_str();
	return Effekseer::Effect::Create(manager_, efkPath.c_str(), magnification, matPtr);
}



Effekseer::Handle EffectSystem::Play(VfxComponent* comp, float x, float y, float z)
{
	// Manager::Play :contentReference[oaicite:15]{index=15}
	Effekseer::EffectRef& effect = comp->mVfx->mEffect;
	Effekseer::Handle efkHandle = manager_->Play(effect, x, y, z);
	comp->mIsPlaying = true;
	return efkHandle;
}

Effekseer::Handle EffectSystem::Play(VfxComponent* comp, const Effekseer::Vector3D& position)
{
	Effekseer::EffectRef& effect = comp->mVfx->mEffect;
	Effekseer::Handle efkHandle = manager_->Play(effect, position);
	comp->mIsPlaying = true;
	return efkHandle;
}

//void EffectSystem::StopEffect()
//{
//	if (handle != -1 && manager != nullptr)
//	{
//		manager->StopEffect(handle);
//		handle = -1;
//		OutputDebugStringA("Effect stopped!!\n");
//	}
//}


void EffectSystem::BeginFrame(ID3D12GraphicsCommandList* dxCmdList)
{
	//if (!platform->NewFrame())
	//	return;

	// SingleFrameMemoryPool은 프레임마다 리셋하는 개념(이름 그대로) :contentReference[oaicite:16]{index=16}
	if (memoryPool_ != nullptr) memoryPool_->NewFrame();

	// DX12 CommandList 브릿지 :contentReference[oaicite:17]{index=17}
	EffekseerRendererDX12::BeginCommandList(commandList_, dxCmdList);


  EffekseerRendererDX12::BeginCommandList(commandList_, GRAPHICS_CMD_LIST.Get());
	renderer_->SetCommandList(commandList_);
}

void EffectSystem::Update(float deltaTime)
{
	std::vector<Entity> effect = mWorld->GetEntitiesWithComponent<VfxComponent>();
	
	for (Entity& e : effect)
	{
		VfxComponent* effectComp = mWorld->GetComponent<VfxComponent>(e);
		if (effectComp == nullptr) continue;
		if (!effectComp->mIsPlaying && effectComp->mTotalTime > 5.0f) {
			Play(effectComp, -10,30,0);
		}
		TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);
		if (tr != nullptr) {
			effectComp->SetPosition(tr->mWorldPosition.x, tr->mWorldPosition.y, tr->mWorldPosition.z);
		}
		manager_->AddLocation(efkHandle, effectComp->mPosition);
		/*Effekseer::Manager::UpdateParameter updateParameter;
		updateParameter.DeltaFrame = 1.0f;*/
		manager_->Update(deltaTime*60.f);
		effectComp->mTotalTime += deltaTime;
		renderer_->SetTime(effectComp->mTotalTime);	// total
	}
}

void EffectSystem::Render(const Effekseer::Matrix44& camera, const Effekseer::Matrix44& projection)
{

	{
	
		renderer_->SetCameraMatrix(camera);
		renderer_->SetProjectionMatrix(projection);
		
		
		if (renderer_->BeginRendering())
		{
			
			Effekseer::Manager::DrawParameter drawParameter;
			drawParameter.ZNear = 0.0f;
			drawParameter.ZFar = 1.0f;
			drawParameter.ViewProjectionMatrix = renderer_->GetCameraProjectionMatrix();
			manager_->Draw(drawParameter);

			renderer_->EndRendering();
		}
	}
}

void EffectSystem::EndFrame()
{

	EffekseerRendererDX12::EndCommandList(commandList_);
	EffekseerRendererDX12::ExecuteCommandList(commandList_);
}

void EffectSystem::Update()
{
	std::vector<Entity> camera{ mWorld->GetEntitiesWithComponent<MainCameraComponent>()[0] };
	CameraComponent* mCamera = mWorld->GetComponent<CameraComponent>(camera[0]);

	Effekseer::Matrix44 cameraMat = ToEfkMatrix(mCamera->GetViewMatrix());
	Effekseer::Matrix44 projMat = ToEfkMatrix(mCamera->GetProjectionMatrix());


	BeginFrame(GRAPHICS_CMD_LIST.Get());
	Render(cameraMat, projMat);
	EndFrame();

}

void EffectSystem::LoadResources()
{
	auto& resources = RESOURCEMANAGER.GetAllResources<Vfx>();
	for (auto& res : resources)
	{
		shared_ptr<Vfx> effect = static_pointer_cast<Vfx>(res.second);
		if (effect)
		{
			effect->mEffect = LoadEffect(ws2s(effect->mEffectPath));
		}
	}
}
