#include "pch.h"
#include "UIRenderSystem.h"
#include <DirectXTK12/CommonStates.h>
#include "Engine.h"
#include "CommandQueue.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Mesh.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "UIEffectPass.h"
#include "AudioVisualizerPass.h"
#include "Timer.h"

UIRenderSystem::UIRenderSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Render;
    mOrder = 2;  // EffectSystem(1) 이후, UIEffectSystem(3) 이전
}

UIRenderSystem::~UIRenderSystem()
{
    GRAPHICS_CMD_QUEUE->WaitForGpuComplete();
    mSpriteBatch.reset();
}

void UIRenderSystem::Initialize()
{
	mQuadMesh = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
    InitializeFont();

    mUIEffectPass = make_shared<UIEffectPass>();
    mUIEffectPass->Initialize(mWorld);

    mVisualizerPass = make_shared<AudioVisualizerPass>();
    mVisualizerPass->Initialize(mWorld);
}

void UIRenderSystem::InitializeFont()
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = Graphics_DescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuhandle, (static_cast<uint32>(UI_INDEX_START))*srvSize);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics_DescHeap->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), (static_cast<uint32>(UI_INDEX_START)) * srvSize);
    
    DirectX::ResourceUploadBatch resourceUpload(DEVICE.Get());
    resourceUpload.Begin();

    RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT);

    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::NonPremultiplied);
    mSpriteBatch = std::make_shared<SpriteBatch>(DEVICE.Get(), resourceUpload, pd);

    mDefaultFont = std::make_shared<SpriteFont>(DEVICE.Get(), resourceUpload, L"..\\Resources\\Font\\myfile.spritefont", cpuDescriptor, gpuDescriptor);
    mDefaultFont->SetDefaultCharacter(L'?');

    for (Entity a : mWorld->View<UITextComponent>()) {
        auto textComp = mWorld->GetComponent<UITextComponent>(a);
        if (textComp == nullptr)
            continue;

        textComp->mFont = mDefaultFont;
        //textComp->m_font.reset();

        auto size = RENDERMANAGER.GetWindow();
        textComp->mFontPos.x = float(size.Width) / 2.f;
        textComp->mFontPos.y = float(size.Height) / 2.f;
    }

    for (Entity a : mWorld->View<UISpriteComponent>()) {
        auto textComp = mWorld->GetComponent<UISpriteComponent>(a);

    }

    auto uploadResourcesFinished = resourceUpload.End(GRAPHICS_CMD_QUEUE->GetCommandQueue().Get());
    uploadResourcesFinished.wait();


}

void UIRenderSystem::Update()
{
    if (false == mWorld->HasComponentPool<MainCameraComponent>())return;

    // UI 전체를 SwapChain RT에 렌더링 (ToneMap 이후)
    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);

    CustomSpriteUpdate();

    RENDERMANAGER.GetGraphicsMemory()->Commit(GRAPHICS_CMD_QUEUE->GetCommandQueue().Get());
    TextUpdate();
    SpriteUpdate();

    mUIEffectPass->Execute(DELTA_TIME);
    RENDERMANAGER.GetGraphicsMemory()->Commit(GRAPHICS_CMD_QUEUE->GetCommandQueue().Get());
    TextUpdate();
}

void UIRenderSystem::TextUpdate()
{
    if (mSpriteBatch == nullptr)
		return;
    if (mDefaultFont == nullptr)
        return;
    
    if (false == mWorld->HasComponentPool<UITextComponent>())
        return;
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITextComponent>() };

    mSpriteBatch->SetViewport(RENDERMANAGER.GetViewPort());
    mSpriteBatch->Begin(GRAPHICS_CMD_LIST.Get());

    for (Entity a : entitys) {
        auto textComp = mWorld->GetComponent<UITextComponent>(a);
        if (textComp == nullptr || textComp->mFont == nullptr)
            continue;

        textComp->mFont = mDefaultFont;
        
		auto posComp = mWorld->GetComponent<UITransformComponent>(a);
        if(posComp) {
            textComp->mFontPos = posComp->mFinalPixelPos;
		}

        std::wstring& output = textComp->mText;

        // mPivot 기반 origin 계산 — mFinalPixelPos가 pivot 기준점이 되도록
        XMVECTOR textSizeVec = mDefaultFont->MeasureString(output.c_str());
        XMFLOAT2 textSize;
        XMStoreFloat2(&textSize, textSizeVec);
        XMFLOAT2 origin = { textSize.x * posComp->mPivot.x,
                             textSize.y * posComp->mPivot.y };

        mDefaultFont->DrawString(mSpriteBatch.get(), output.c_str(),
            textComp->mFontPos, Colors::White, 0.f, origin);
    }

    mSpriteBatch->End();
}

void UIRenderSystem::CustomSpriteUpdate()
{
    mInstances.clear();

    if (false == mWorld->HasComponentPool<UICusSpriteComponent>()) return;

    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<UITransformComponent, UICusSpriteComponent>() };

    for (auto& e : entitys)
    {
        auto tr = mWorld->GetComponent<UITransformComponent>(e);
        auto sp = mWorld->GetComponent<UICusSpriteComponent>(e);

        if (!sp->mVisible)
            continue;

        UIInstanceData data;
        data.Position = tr->mFinalPixelPos;
        data.Size = tr->mFinalSize;
        data.Pivot = tr->mPivot;
        data.MaterialIndex = sp->mMaterial->GetIndex();
        data.ZOrder = static_cast<float>(tr->mUILayerIndex);

        mInstances.push_back(data);
    }

    // ── 비주얼라이저 바 데이터를 인스턴스 벡터 뒤에 추가 ──────────────
    // UIInfo 버퍼 레이아웃: [일반 UI (0..N-1)] [바 데이터 (N..N+M-1)]
    // DrawIndexedInstanced의 StartInstanceLocation으로 각 파트를 독립적으로 드로우
    uint32 regularCount = static_cast<uint32>(mInstances.size());
    if (mVisualizerPass)
        mVisualizerPass->AppendBarInstances(mInstances);
    uint32 barCount = static_cast<uint32>(mInstances.size()) - regularCount;

    UploadInstanceBuffer();

    // 일반 UI 스프라이트 드로우 (startInstance = 0)
    InstancingRender(regularCount, 0);

    // 비주얼라이저 바 드로우 (startInstance = regularCount, 전용 셰이더)
    if (mVisualizerPass && barCount > 0)
        mVisualizerPass->Execute(regularCount, barCount);
}

void UIRenderSystem::SpriteUpdate()
{
    if (false == mWorld->HasComponentPool<UISpriteComponent>())
        return;
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UISpriteComponent>() };
    mSpriteBatch->SetViewport(RENDERMANAGER.GetViewPort());
    mSpriteBatch->Begin(GRAPHICS_CMD_LIST.Get());
    for (Entity a : entitys) {
        auto spriteComp = mWorld->GetComponent<UISpriteComponent>(a);
        auto transComp = mWorld->GetComponent<UITransformComponent>(a);
      
        if (!spriteComp->mVisible || spriteComp->mTexture == nullptr)
            continue;
        XMUINT2 textureSize;
        if (spriteComp->mIsAnimated && spriteComp->mTextures.empty() && spriteComp->mFrameCount > 1)
        {
            textureSize = XMUINT2(
                static_cast<uint32_t>(spriteComp->mTexture->GetWidth()),
                static_cast<uint32_t>(spriteComp->mTexture->GetHeight())
            );
           
        }
        else
        {
            textureSize = XMUINT2(
                static_cast<uint32_t>(transComp->mFinalSize.x),
                static_cast<uint32_t>(transComp->mFinalSize.y)
            );
		}


        RECT sourceRect{};
        RECT* sourceRectPtr = nullptr;

        if (spriteComp->mIsAnimated && spriteComp->mTextures.empty() && spriteComp->mFrameCount > 1)
        {
            sourceRect = spriteComp->GetCurrentFrameRect();
            sourceRectPtr = &sourceRect;
        }

        // mPivot 기반 origin 계산 — mFinalPixelPos가 pivot 기준점이 되도록
        // sourceRect 있으면 프레임 크기 기준, 없으면 textureSize(=mSize) 기준
        XMFLOAT2 origin;
        if (sourceRectPtr != nullptr)
        {
            origin = { transComp->mPivot.x * (float)(sourceRect.right  - sourceRect.left),
                       transComp->mPivot.y * (float)(sourceRect.bottom - sourceRect.top) };
        }
        else
        {
            origin = { transComp->mPivot.x * (float)textureSize.x,
                       transComp->mPivot.y * (float)textureSize.y };
        }

        mSpriteBatch->Draw(
            spriteComp->mTexture->GetSrvGpuHandle(),
            textureSize,
            transComp->mFinalPixelPos,
            sourceRectPtr,
            Colors::White,
            0.f,    // rotation
            origin  // pivot 적용
        );
    }
    mSpriteBatch->End();
}

void UIRenderSystem::UploadInstanceBuffer()
{
    if (mInstances.empty())
        return;
    uint32 frameIdx = RENDERMANAGER.GetFrameResourceIndex();
    RENDERMANAGER.GetGroupBuffer(frameIdx)->UIInfo->PushGraphicsData(
        mInstances.data(),
        static_cast<uint32>(sizeof(UIInstanceData) * mInstances.size()));
}

void UIRenderSystem::InstancingRender(uint32 count, uint32 startInstance)
{
    if (count == 0)
        return;
    RESOURCEMANAGER.Get<Shader>(L"UI")->Update();
    // startInstance → DrawIndexedInstanced의 StartInstanceLocation
    // SV_InstanceID = startInstance + 0 ~ startInstance + count - 1
    mQuadMesh->Render(count, 0, 0, startInstance);
}
