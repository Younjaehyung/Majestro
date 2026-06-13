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
#include "UIHpBarUpdateFeature.h"
#include "UIFeature.h"
#include "UIAudioVisualizerFeature.h"
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

   
    RENDERMANAGER.SetGraphicsTable();

    CustomSpriteRender();
    TextUpdate();
    SpriteUpdate();
    PostSpriteRender();

    mUIEffectPass->Execute(DELTA_TIME);


    RENDERMANAGER.SetGraphicsTable();
    TextUpdate();

}

// Update
void UIRenderSystem::CustomSpriteRender()
{
    mInstances.clear();

    if (mWorld->HasComponentPool<UICusSpriteComponent>())
        RenderCustomSprite();

    const uint32 regularCount = static_cast<uint32>(mInstances.size());

    if (mFeatures != nullptr)
    {
        for (const auto& spritePass : *mFeatures)
        {
            if (spritePass != nullptr)
                spritePass->CustomSpriteRender(mInstances);
        }
    }

    UploadInstanceBuffer();

    // 일반 UI 스프라이트 드로우
    InstancingRender(regularCount, 0);
}

void UIRenderSystem::SpriteUpdate()
{
    if (false == mWorld->HasComponentPool<UISpriteComponent>())
        return;
    
    mSpriteBatch->SetViewport(RENDERMANAGER.GetViewPort());
    mSpriteBatch->Begin(GRAPHICS_CMD_LIST.Get());

    RenderSpirte();

    if (mFeatures != nullptr){
        for (const auto& spritePass : *mFeatures)
        {
            if (spritePass != nullptr)
                spritePass->SpriteRender(mSpriteBatch.get());
        }
    }

    mSpriteBatch->End();
}

void UIRenderSystem::PostSpriteRender()
{
    if (mFeatures == nullptr)
        return;

    RENDERMANAGER.SetGraphicsTable();

    if (mFeatures != nullptr) {
        for (const auto& feature : *mFeatures)
        {
            if (feature != nullptr)
                feature->PostSpriteRender(mInstances);
        }
    }
}

void UIRenderSystem::TextUpdate()
{
    if (mSpriteBatch == nullptr || mDefaultFont == nullptr)
		return;
    
    if (false == mWorld->HasComponentPool<UITextComponent>())
        return;

   

    mSpriteBatch->SetViewport(RENDERMANAGER.GetViewPort());
    mSpriteBatch->Begin(GRAPHICS_CMD_LIST.Get());

    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITextComponent>() };

    for (Entity a : entitys) {
        auto textComp = mWorld->GetComponent<UITextComponent>(a);
        if (textComp == nullptr)
            continue;

        if (!textComp->mVisible)
            continue;

        // InitializeFont() 이후에 생성된 엔티티(런타임 스폰)도 폰트를 할당
        if (textComp->mFont == nullptr)
            textComp->mFont = mDefaultFont;
        
		auto posComp = mWorld->GetComponent<UITransformComponent>(a);
        if(posComp) {
            textComp->mFontPos = posComp->mFinalPixelPos;
		}

        std::wstring& output = textComp->mText;

        // mPivot 기반 origin 계산 — mFinalPixelPos가 pivot 기준점이 되도록.
        // posComp가 없으면 좌상단(원점) 기준으로 그린다.
        const DirectX::SimpleMath::Vector2 pivot = posComp ? posComp->mPivot : Vec2(0.f, 0.f);
        const float scale = posComp ? posComp->mScale.x : 1.f;
        XMVECTOR textSizeVec = mDefaultFont->MeasureString(output.c_str());
        XMFLOAT2 textSize;
        XMStoreFloat2(&textSize, textSizeVec);
        XMFLOAT2 origin = { textSize.x * pivot.x,
                             textSize.y * pivot.y };

        // 외곽선(아웃라인) - 사실상 글자들 여러번 쓰기임
        if (textComp->mOutlineThickness > 0.f)
        {
            const float th = textComp->mOutlineThickness;
            static const DirectX::SimpleMath::Vector2 kDirs[8] = {
                { -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f },
                { -1.f,  0.f },                { 1.f,  0.f },
                { -1.f,  1.f }, { 0.f,  1.f }, { 1.f,  1.f },
            };

            // 외곽선 알파는 본체 알파를 따라감
            DirectX::XMVECTORF32 outlineColor = textComp->mOutlineColor;

            outlineColor.f[3] *= textComp->mColor.f[3];
            for (const auto& d : kDirs)
            {
                const DirectX::SimpleMath::Vector2 offsetPos = textComp->mFontPos + d * th;
                mDefaultFont->DrawString(mSpriteBatch.get(), output.c_str(),
                    offsetPos, outlineColor, textComp->mRotation, origin, scale);
            }
        }

        mDefaultFont->DrawString(mSpriteBatch.get(), output.c_str(),
            textComp->mFontPos, textComp->mColor, textComp->mRotation, origin, scale);
    }

    mSpriteBatch->End();
}


// Render


void UIRenderSystem::RenderCustomSprite()
{
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
}

void UIRenderSystem::RenderSpirte()
{
   
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UISpriteComponent>() };

    SortSpriteLayer(entitys);

    for (Entity a : entitys) {
        auto spriteComp = mWorld->GetComponent<UISpriteComponent>(a);
        auto transComp = mWorld->GetComponent<UITransformComponent>(a);

        if (spriteComp == nullptr || transComp == nullptr)
            continue;

        if (!spriteComp->mVisible || spriteComp->mTexture == nullptr)
            continue;

        XMUINT2 textureSize;
        RECT sourceRect{};
        RECT* sourceRectPtr = nullptr;

        if (spriteComp->mIsAnimated && spriteComp->mTextures.empty() && spriteComp->mFrameCount > 1)
        {   // 애니메이션 프레임이 텍스처 시트 방식
            textureSize = XMUINT2(
                static_cast<uint32_t>(spriteComp->mTexture->GetWidth()),
                static_cast<uint32_t>(spriteComp->mTexture->GetHeight())
            );


            sourceRect = spriteComp->GetCurrentFrameRect();
            sourceRectPtr = &sourceRect;
        }
        else if (spriteComp->mUseSourceRect)
        {    // 아틀라스
            textureSize = XMUINT2(
                static_cast<uint32_t>(spriteComp->mTexture->GetWidth()),
                static_cast<uint32_t>(spriteComp->mTexture->GetHeight())
            );

            sourceRect = spriteComp->mSourceRect;
            sourceRectPtr = &sourceRect;
        }
        else if (spriteComp->mUseVisibleRange)
        {    // visible range를 사용
            textureSize = XMUINT2(
                static_cast<uint32_t>(spriteComp->mTexture->GetWidth()),
                static_cast<uint32_t>(spriteComp->mTexture->GetHeight())
            );

            sourceRect = RECT{
               0,
               0,
               static_cast<LONG>(textureSize.x),
               static_cast<LONG>(textureSize.y)
            };
            sourceRectPtr = &sourceRect;
        }
        else
        {
            textureSize = XMUINT2(
                static_cast<uint32_t>(transComp->mFinalSize.x),
                static_cast<uint32_t>(transComp->mFinalSize.y)
            );
        }

        // 가시 구간 크롭
        LONG baseWidth = 0;
        LONG baseHeight = 0;
        if (spriteComp->mUseVisibleRange && sourceRectPtr != nullptr)
        {
            if (!spriteComp->mVisibleRangeVertical)
            {   // 가로(X) 크롭
                const LONG fullWidth = sourceRect.right - sourceRect.left;
                baseWidth = fullWidth;
                LONG startOffset = 0;
                LONG endOffset = fullWidth;

                if (spriteComp->mVisibleRangeUsePixels)
                {
                    startOffset = static_cast<LONG>(spriteComp->mVisibleStartX);
                    endOffset = static_cast<LONG>(spriteComp->mVisibleEndX);
                }
                else
                {
                    startOffset = static_cast<LONG>(fullWidth * spriteComp->mVisibleStartX);
                    endOffset = static_cast<LONG>(fullWidth * spriteComp->mVisibleEndX);
                }

                sourceRect.left += std::clamp(startOffset, 0L, fullWidth);
                sourceRect.right = sourceRect.left + std::clamp(endOffset - startOffset, 0L, fullWidth);
            }
            else
            {   // 세로(Y) 크롭
                const LONG fullHeight = sourceRect.bottom - sourceRect.top;
                baseHeight = fullHeight;
                const LONG startOffset = static_cast<LONG>(fullHeight * spriteComp->mVisibleStartY);
                const LONG endOffset   = static_cast<LONG>(fullHeight * spriteComp->mVisibleEndY);

                sourceRect.top    += std::clamp(startOffset, 0L, fullHeight);
                sourceRect.bottom  = sourceRect.top + std::clamp(endOffset - startOffset, 0L, fullHeight);
            }
        }

        // mPivot 기반 origin 계산 — mFinalPixelPos가 pivot 기준점이 되도록
        // sourceRect 있으면 프레임 크기 기준, 없으면 textureSize(=mSize) 기준
        XMFLOAT2 origin;
        if (sourceRectPtr != nullptr)
        {
            origin = { transComp->mPivot.x * (float)(sourceRect.right - sourceRect.left),
                       transComp->mPivot.y * (float)(sourceRect.bottom - sourceRect.top) };
        }
        else
        {
            origin = { transComp->mPivot.x * (float)textureSize.x,
                       transComp->mPivot.y * (float)textureSize.y };
        }

        if ((spriteComp->mUseVisibleRange || spriteComp->mUseSourceRect) && sourceRectPtr != nullptr)
        {
            const bool vertical = spriteComp->mUseVisibleRange && spriteComp->mVisibleRangeVertical;
            const LONG croppedDim = vertical ? (sourceRect.bottom - sourceRect.top)
                                             : (sourceRect.right - sourceRect.left);
            const LONG baseDim = vertical ? baseHeight : baseWidth;
            float visibleRatio = 1.f;
            if (spriteComp->mUseVisibleRange && baseDim > 0)
                visibleRatio = std::clamp(static_cast<float>(croppedDim) / static_cast<float>(baseDim), 0.f, 1.f);


            // VisibleRange가 켜져 있으면 그쪽 keep 플래그를 따른다
            // VisibleRange 없이 아틀라스만이면 원본 크기 유지.
            const bool keepSize = spriteComp->mUseVisibleRange
                ? spriteComp->mVisibleRangeKeepDestinationSize : spriteComp->mUseSourceRect;
            const float destWidth  = (!vertical && !keepSize)  ? transComp->mFinalSize.x * visibleRatio : transComp->mFinalSize.x;
            const float destHeight = ( vertical && !keepSize)  ? transComp->mFinalSize.y * visibleRatio : transComp->mFinalSize.y;

            // rotation=0 이면 기존과 동일한 위치
            const LONG left = static_cast<LONG>(transComp->mFinalPixelPos.x);
            const LONG top = static_cast<LONG>(transComp->mFinalPixelPos.y);
            const LONG right = left + static_cast<LONG>(destWidth);
            const LONG bottom = top + static_cast<LONG>(destHeight);
            const RECT destRect{ left, top, right, bottom };

            mSpriteBatch->Draw(
                spriteComp->mTexture->GetSrvGpuHandle(),
                textureSize,
                destRect,
                sourceRectPtr,
                spriteComp->mColorTint,
                spriteComp->mRotation,
                origin
            );
        }
        else
        {
            mSpriteBatch->Draw(
                spriteComp->mTexture->GetSrvGpuHandle(),
                textureSize,
                transComp->mFinalPixelPos,
                sourceRectPtr,
                spriteComp->mColorTint,
                spriteComp->mRotation,
                origin  // pivot 적용
            );
        }
    }

}


void UIRenderSystem::SortSpriteLayer(std::vector<Entity>& entitys)
{
    // 같은 레이어 안에서는 기존 생성 순서를 유지
    std::stable_sort(entitys.begin(), entitys.end(),
        [this](Entity lhs, Entity rhs)
        {
            const UITransformComponent* ltr = mWorld->GetComponent<UITransformComponent>(lhs);
            const UITransformComponent* rtr = mWorld->GetComponent<UITransformComponent>(rhs);

            const uint8 lhsLayer = ltr ? ltr->mUILayerIndex : 0;
            const uint8 rhsLayer = rtr ? rtr->mUILayerIndex : 0;

            return lhsLayer < rhsLayer;
        });
}

void UIRenderSystem::UploadInstanceBuffer()
{
    if (mInstances.empty())
        return;
    uint32 frameIdx = RENDERMANAGER.GetFrameResourceIndex();
    RENDERMANAGER.GetGroupBuffer(frameIdx)->UIInfo->PushGraphicsData(
        mInstances.data(), static_cast<uint32>(sizeof(UIInstanceData) * mInstances.size()));
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
