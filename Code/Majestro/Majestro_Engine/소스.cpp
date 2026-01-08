#include "pch.h"
#include "EffectRenderer.h"
#include "DX12Core.h"
#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"

EffectRenderer::EffectRenderer()
    : handle(-1), totalTime(0.0f)
{
}

EffectRenderer::~EffectRenderer()
{
    if (handle != -1 && manager != nullptr)
    {
        manager->StopEffect(handle);
    }
}

void EffectRenderer::Initialize(DX12Core& core)
{
    // Effekseer �ʱ�ȭ
    manager = Effekseer::Manager::Create(8000);
    manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

    efDevice = EffekseerRendererDX12::CreateGraphicsDevice(core.GetDevice(), core.GetCmdQueue(), SWAP_CHAIN_BUFFER_COUNT);

    DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    renderer = EffekseerRendererDX12::Create(
        efDevice,                   // Device, cmdQueue, swapchain buff count
        &renderTargetFormat,        // renderTargetFormats (Forward Pass - �����)
        1,                          // renderTargetCount (Forward�� 1��)
        DXGI_FORMAT_D32_FLOAT,      // depthFormat 
        false,                      // isReversedDepth
        8000                        // squareMaxCount
    );

    efMemPool = EffekseerRenderer::CreateSingleFrameMemoryPool(efDevice);

    efCmdList = EffekseerRenderer::CreateCommandList(efDevice, efMemPool);

    manager->SetModelRenderer(renderer->CreateModelRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());

    manager->SetTextureLoader(renderer->CreateTextureLoader());
    manager->SetModelLoader(renderer->CreateModelLoader());
    manager->SetMaterialLoader(renderer->CreateMaterialLoader());
    manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

    OutputDebugStringA("EffectRenderer initialized!\n");
}

void EffectRenderer::Update(float deltaTime)
{
    if (manager == nullptr)
        return;

    if (handle >= 0)
    {
        if (auto tr = GetGameObject()->GetComponent<Transform>())
        {
            XMFLOAT3 p = tr->GetPosition();
            manager->SetLocation(handle, p.x, p.y, p.z);
        }
    }

    totalTime += deltaTime;
    manager->Update(deltaTime * 60.0f);
}

void EffectRenderer::Render(DX12Core& core, Camera* camera)
{
    if (renderer == nullptr || manager == nullptr || camera == nullptr)
        return;

    XMFLOAT3 cameraPos = camera->GetPosition();
    XMFLOAT3 cameraTarget = camera->GetTargetPosition();

    auto viewerPosition = ::Effekseer::Vector3D(cameraPos.x, cameraPos.y, cameraPos.z);
    auto targetPosition = ::Effekseer::Vector3D(cameraTarget.x, cameraTarget.y, cameraTarget.z);

    Effekseer::Matrix44 projectionMatrix;
    float aspectRatio = static_cast<float>(WinSize.x) / static_cast<float>(WinSize.y);
    projectionMatrix.PerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);

    Effekseer::Matrix44 cameraMatrix;
    cameraMatrix.LookAtLH(viewerPosition, targetPosition, ::Effekseer::Vector3D(0.0f, 1.0f, 0.0f));

    Effekseer::Manager::LayerParameter layerParameter;
    layerParameter.ViewerPosition = viewerPosition;
    manager->SetLayerParameter(0, layerParameter);

    renderer->SetTime(totalTime);
    renderer->SetProjectionMatrix(projectionMatrix);
    renderer->SetCameraMatrix(cameraMatrix);

    efMemPool->NewFrame();

    ID3D12GraphicsCommandList* dx12Cmd = core.GetGraphicsCmdList();

    EffekseerRendererDX12::BeginCommandList(efCmdList, dx12Cmd);

    renderer->SetCommandList(efCmdList);

    renderer->BeginRendering();

    Effekseer::Manager::DrawParameter drawParameter;
    drawParameter.ZNear = 0.1f;
    drawParameter.ZFar = 1000.0f;
    drawParameter.ViewProjectionMatrix = renderer->GetCameraProjectionMatrix();
    manager->Draw(drawParameter);

    renderer->EndRendering();

    EffekseerRendererDX12::EndCommandList(efCmdList);
}

void EffectRenderer::LoadEffect(const char16_t* effectPath)
{
    effect = Effekseer::Effect::Create(manager, effectPath);

    if (effect != nullptr)
    {
        OutputDebugStringA("Effect loaded successfully\n");
    }
    else
    {
        OutputDebugStringA("Failed to load effect\n");
    }
}

void EffectRenderer::PlayEffect()
{
    if (effect == nullptr || manager == nullptr) {
        OutputDebugStringA("No effect loaded!\n");
        return;
    }

    auto transform = GetGameObject()->GetComponent<Transform>();

    XMFLOAT3 pos = transform ? transform->GetPosition() : XMFLOAT3{ 0,0,0 };

    handle = manager->Play(effect, 0, 0, 0);
    manager->SetLocation(handle, pos.x, pos.y, pos.z);
}

void EffectRenderer::StopEffect()
{
    if (handle != -1 && manager != nullptr)
    {
        manager->StopEffect(handle);
        handle = -1;
        OutputDebugStringA("Effect stopped!!\n");
    }
}