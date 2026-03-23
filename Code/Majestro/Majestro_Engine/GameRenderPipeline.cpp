#include "pch.h"
#include "GameRenderPipeline.h"


#include "DepthPrePass.h"
#include "ShadowPass.h"
#include "GBufferPass.h"
#include "LightsPass.h"
#include "ForwardPass.h"
#include "OutlinePass.h"
#include "EffectPass.h"
#include "RenderPass.h"     
#include "MotionVectorPass.h"
#include "FogPass.h"
#include "MotionBlurPass.h"
#include "LuminancePass.h"
#include "ToneMapPass.h"


#include "Engine.h"
#include "RenderManager.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "World.h"
#include "CameraComponent.h"
#include "PlayerComponent.h" 
#include "TagComponent.h"


void GameRenderPipeline::Initialize(World* world)
{
    mWorld = world;


    mDepthPrePass    = make_shared<DepthPrePass>();
    mShadowPass      = make_shared<ShadowPass>();
    mGBufferPass     = make_shared<GBufferPass>();
    mLightPass       = make_shared<LightsPass>();
    mForwardPass     = make_shared<ForwardPass>();
    mOutlinePass     = make_shared<OutlinePass>();
    mEffectPass      = make_shared<EffectPass>();
    mPostProcessPass = make_shared<PostProcessPass>();

    mDepthPrePass->Initialize();
    mEffectPass->Initialize(world);
    mPostProcessPass->Initialize();

    mMotionVectorPass = make_shared<MotionVectorPass>();
    mMotionVectorPass->Initialize();


    mMotionBlurPass = make_shared<MotionBlurPass>();
    mPostProcessPass->AddHDRPass(mMotionBlurPass);

    mFogPass = make_shared<FogPass>();
    mPostProcessPass->AddHDRPass(mFogPass);

    mLuminancePass = make_shared<LuminancePass>();
    // mPostProcessPass->AddLDRPass(mLuminancePass);
}

void GameRenderPipeline::OnResize(uint32 w, uint32 h)
{
    
}

void GameRenderPipeline::SetupPassTable(
    std::array<PassCustomData,
    static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& table)
{
    mDepthPrePass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH);

    mShadowPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::SHADOW);

    mGBufferPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::SHADOW,
        RENDER_TARGET_GROUP_TYPE::G_BUFFER);

    mLightPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::LIGHTING);

    mForwardPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::HDR);

    mOutlinePass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::HDR,
        RENDER_TARGET_GROUP_TYPE::HDR);

    mEffectPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::HDR);

    mMotionVectorPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::PRE_DEPTH,
        RENDER_TARGET_GROUP_TYPE::MOTION_VECTOR);

    mPostProcessPass->SetData(table);
}

void GameRenderPipeline::PreCompute(const RenderContext& ctx)
{
    mForwardPass->Compute();
}


void GameRenderPipeline::Execute(const RenderContext& ctx)
{
    UpdatePassStates();

    if (!mIsPaused) // 인게임 풀 파이프라인
    {
        
        RenderDepthPrePass(ctx);
        RenderShadow(ctx);
        RenderDeferred(ctx);
        RenderForward(ctx);
        RenderOutline(ctx);
        RenderEffect(ctx);
        RenderPost(ctx);
    }
    else
    {
        // PauseMenu: 게임 월드는 마지막 프레임 HDR 유지, UI 오버레이만
        // ToneMap + FinalComposite만 실행해 기존 프레임버퍼 출력
        RenderPost(ctx);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderPipeline::UpdatePassStates()
{
    if (!mMotionBlurPass)
        return;
    if (!mWorld->HasComponentPool<LocalPlayerComponent>())
        return;

    bool enableBlur = false;
    auto players = mWorld->GetEntitiesWithComponent<LocalPlayerComponent>();
    for (auto e : players)
    {
        auto* player = mWorld->GetComponent<MainPlayerComponent>(e);
        if (player)
        {
            enableBlur = (player->mStatePacket == S_Dash);
        }
        break;
    }
    mMotionBlurPass->SetEnabled(enableBlur);
}

// ─────────────────────────────────────────────────────────────────────────────
// 제어 API
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderPipeline::SetMotionBlurEnabled(bool on)
{
    if (mMotionBlurPass) mMotionBlurPass->SetEnabled(on);
}

void GameRenderPipeline::SetFogEnabled(bool on)
{
    if (mFogPass) mFogPass->SetEnabled(on);
}

void GameRenderPipeline::SetOutlineEnabled(bool on)
{
    if (mOutlinePass) mOutlinePass->SetEnabled(on);
}

void GameRenderPipeline::AddHDREffect(shared_ptr<RenderPass> pass)
{
    if (mPostProcessPass) mPostProcessPass->AddHDRPass(pass);
}

void GameRenderPipeline::RemoveHDREffect(shared_ptr<RenderPass> pass)
{
    if (mPostProcessPass) mPostProcessPass->RemoveHDRPass(pass);
}





void GameRenderPipeline::RenderDepthPrePass(const RenderContext& ctx)
{
    mDepthPrePass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderShadow(const RenderContext& ctx)
{
    mShadowPass->Execute(
        *ctx.deferredBatchs,
        *ctx.shadowBatchs,
        const_cast<std::array<bool, 4>&>(*ctx.cascadeActive));
}

void GameRenderPipeline::RenderDeferred(const RenderContext& ctx)
{
   
    mGBufferPass->Execute(*ctx.deferredBatchs);


    mLightPass->Execute(*ctx.lightBatchs);


    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

    if (RENDERMANAGER.IsMsaaEnabled())
    {
        RENDERMANAGER.GetRenderTargetGroup(
            static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN))
            .OMSetRenderTargets(1, backIndex);
    }

    auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
    hdrGroup.OMSetRenderTargets();

    RESOURCEMANAGER.Get<Shader>(L"Final")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    if (RENDERMANAGER.IsMsaaEnabled())
    {
        RENDERMANAGER.GetRenderTargetGroup(
            static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN))
            .WaitTargetToResource();
    }
    hdrGroup.WaitTargetToResource();


    mMotionVectorPass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderForward(const RenderContext& ctx)
{
    mForwardPass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderOutline(const RenderContext& ctx)
{
    mOutlinePass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderEffect(const RenderContext& ctx)
{
    if (!ctx.camera) return;

    float dt = ctx.deltaTime;
    Effekseer::Matrix44 viewMat = mEffectPass->ToEfkMatrix(ctx.camera->GetViewMatrix());
    Effekseer::Matrix44 projMat = mEffectPass->ToEfkMatrix(ctx.camera->GetProjectionMatrix());
    mEffectPass->Execute(dt, viewMat, projMat);

    // Effekseer가 RootSignature/DescriptorHeap을 변경하므로 엔진 상태 복원
    RENDERMANAGER.SetGraphicsTable();
}

void GameRenderPipeline::RenderPost(const RenderContext& ctx)
{
    mPostProcessPass->Execute(*ctx.deferredBatchs);
}
