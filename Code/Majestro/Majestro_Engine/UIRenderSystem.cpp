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

UIRenderSystem::UIRenderSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Render;
    mOrder = 1;
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

    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

    if (RENDERMANAGER.IsMsaaEnabled()) {//msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitResourceToTarget();
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
    }
    else
    {
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
    }

    
    CustomSpriteUpdate();

    RENDERMANAGER.GetGraphicsMemory()->Commit(GRAPHICS_CMD_QUEUE->GetCommandQueue().Get());
    TextUpdate();
	SpriteUpdate();

    if (RENDERMANAGER.IsMsaaEnabled()) {//msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();
    }
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

        std::wstring& output = textComp->mText;
        Vec2 origin = mDefaultFont->MeasureString(output.c_str()) / 2.f;

        mDefaultFont->DrawString(mSpriteBatch.get(), output.c_str(),
            textComp->mFontPos, Colors::White, 30.f, origin);
    }

    mSpriteBatch->End();
}

void UIRenderSystem::CustomSpriteUpdate()
{
    mInstances.clear();


    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<UITransformComponent, UICusSpriteComponent>() };

    for (auto& e : entitys)
    {
        auto tr = mWorld->GetComponent<UITransformComponent>(e);
        auto sp = mWorld->GetComponent<UICusSpriteComponent>(e);

        if (!sp->mVisible)
            continue;

        UIInstanceData data;
        data.Position = tr->mFinalPixelPos;
        data.Size = tr->mSize;
        data.Pivot = tr->mPivot;
        data.MaterialIndex = sp->mMaterial->GetIndex();
        data.ZOrder = static_cast<float>(tr->mUILayerIndex);

        mInstances.push_back(data);
    }

    UploadInstanceBuffer();
    InstancingRender();

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
        //auto transformComp = mWorld->GetComponent<UITransformComponent>(a);
        if (!spriteComp->mVisible || spriteComp->mTexture == nullptr)
            continue;
        //spriteComp->m_spriteBatch->SetViewport(viewPort);

        // 1) Int2 → XMUINT2 변환
        XMUINT2 textureSize(
            static_cast<uint32_t>(spriteComp->mSize.x),
            static_cast<uint32_t>(spriteComp->mSize.y)
        );

        // 2) Vec(XMFLOAT3) → XMFLOAT2 변환 (z 성분 버림)
        XMFLOAT2 position(
            spriteComp->mPos.x,
            spriteComp->mPos.y
        );
        RECT sourceRect{};
        RECT* sourceRectPtr = nullptr;

        if (spriteComp->mIsAnimated && spriteComp->mTextures.empty() && spriteComp->mFrameCount > 1)
        {
            sourceRect = spriteComp->GetCurrentFrameRect();
            sourceRectPtr = &sourceRect;
        }
        // 3) color 인자 추가 (흰색 = 원본 색상 그대로)
        mSpriteBatch->Draw(
            spriteComp->mTexture->GetSrvGpuHandle(),
            textureSize,
            position,
            sourceRectPtr,
            Colors::White
        );
    }
    mSpriteBatch->End();
}

void UIRenderSystem::UploadInstanceBuffer()
{
    uint32 mFrameCount = RENDERMANAGER.GetFrameResourceIndex();
    RENDERMANAGER.GetGroupBuffer(mFrameCount)->UIInfo->PushGraphicsData(mInstances.data(), static_cast<uint32>(sizeof(UIInstanceData) * mInstances.size()));

}

void UIRenderSystem::InstancingRender()
{
    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();


    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);


    RESOURCEMANAGER.Get<Shader>(L"UI")->Update();

    mQuadMesh->Render(mInstances.size(), 0, 0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);

}
