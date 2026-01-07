#include "pch.h"
#include "EffectSystem.h"

#include "World.h"
#include "Engine.h"
#include "RenderManager.h"

#include "TagComponent.h"
#include "CameraComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"

EffectSystem::EffectSystem(World* world) : System::System(world)
{
}

EffectSystem::~EffectSystem()
{
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

	effect_ = LoadEffect(u"..\\Resources\\Effect\\Fire\\Fire\\Fire_efc.efkefc");
}

Effekseer::Handle EffectSystem::Play(Effekseer::EffectRef& effect, float x, float y, float z)
{
	// Manager::Play :contentReference[oaicite:15]{index=15}
	return manager_->Play(effect, x, y, z);
}


void EffectSystem::BeginFrame(ID3D12GraphicsCommandList* dxCmdList)
{
	//if (!platform->NewFrame())
	//	return;

	// SingleFrameMemoryPool은 프레임마다 리셋하는 개념(이름 그대로) :contentReference[oaicite:16]{index=16}
	if (memoryPool_ != nullptr) memoryPool_->NewFrame();

	// DX12 CommandList 브릿지 :contentReference[oaicite:17]{index=17}
	EffekseerRendererDX12::BeginCommandList(commandList_, dxCmdList);


	// DX12 CommandList 브릿지 :contentReference[oaicite:17]{index=17}
	LLGI::Color8 color;
	color.R = 0;
	color.G = 0;
	color.B = 0;
	color.A = 255;

	//commandList_->Begin();
	//commandList_->BeginRenderPass(platform->GetCurrentScreen(color, true, false)); // TODO: isDepthClear is false, because it fails with dx12.


  EffekseerRendererDX12::BeginCommandList(commandList_, GRAPHICS_CMD_LIST.Get());
	renderer_->SetCommandList(commandList_);
}

void EffectSystem::Update(float deltaTime)
{

	Play(effect_, 0, 0, 0);
	{
		manager_->AddLocation(efkHandle, ::Effekseer::Vector3D(0.2f, 0.0f, 0.0f));

		Effekseer::Manager::UpdateParameter updateParameter;
		manager_->Update(updateParameter);
		renderer_->SetTime(deltaTime / 60.0f);
	}
}

void EffectSystem::Render(const Effekseer::Matrix44& camera, const Effekseer::Matrix44& projection)
{

	{
		// Renderer는 카메라/프로젝션 행렬을 보관합니다. :contentReference[oaicite:18]{index=18}
		renderer_->SetCameraMatrix(camera);
		renderer_->SetProjectionMatrix(projection);

		// BeginRendering / EndRendering :contentReference[oaicite:19]{index=19}
		if (renderer_->BeginRendering())
		{
			// Draw :contentReference[oaicite:20]{index=20}
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
	// DX12 CommandList 브릿지 :contentReference[oaicite:21]{index=21}
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
	/*gEffekseerManager->EffekseerRendererDX12::SetProjectionMatrix(RENDERMANAGER.GetProjectionMatrix().ToEffekseer());
	gEffekseerManager->Draw();
	gEffekseerRenderer->EndRendering();*/
}

