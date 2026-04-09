#include "pch.h"
#include "LobbyRenderPipeline.h"

#include "DepthPrePass.h"
#include "ShadowPass.h"
#include "ForwardPass.h"
#include "EffectPass.h"
#include "RenderPass.h"   // PostProcessPass
#include "FogPass.h"
#include "ToneMapPass.h"
#include "GBufferPass.h"
#include "LightsPass.h"
#include "OutlinePass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "World.h"
#include "CameraComponent.h"

// ─────────────────────────────────────────────────────────────────────────────
// Initialize / OnResize
// ─────────────────────────────────────────────────────────────────────────────

void LobbyRenderPipeline::Initialize(World* world)
{
    mWorld = world;

    

    mDepthPrePass       = make_shared<DepthPrePass>();
    mShadowPass         = make_shared<ShadowPass>();
    mGBufferPass        = make_shared<GBufferPass>();
    mLightPass          = make_shared<LightsPass>();
    mForwardPass        = make_shared<ForwardPass>();
    mOutlinePass        = make_shared<OutlinePass>();
    mEffectPass         = make_shared<EffectPass>();
    mPostProcessPass    = make_shared<PostProcessPass>();
	

    mEffectPass->Initialize(world);
    mPostProcessPass->Initialize();
    mDepthPrePass->Initialize();
    mShadowPass->Initialize();
    mGBufferPass->Initialize();
    mLightPass->Initialize();
    mForwardPass->Initialize();
    mOutlinePass->Initialize();


    mFogPass = make_shared<FogPass>();
    mPostProcessPass->AddHDRPass(mFogPass);
}

void LobbyRenderPipeline::OnResize(uint32 w, uint32 h)
{
    // 현재 Pass들은 ResizeCallback 없음
}

void LobbyRenderPipeline::SetupPassTable(
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

    mPostProcessPass->SetData(table);
}


void LobbyRenderPipeline::PreCompute(const RenderContext& ctx)
{
    mForwardPass->Compute();
}


void LobbyRenderPipeline::Execute(const RenderContext& ctx)
{

    mDepthPrePass->Execute(*ctx.deferredBatchs);
    RenderDeferred(ctx);
    mForwardPass->Execute(*ctx.deferredBatchs);

    // Effekseer VFX
    if (ctx.camera)
    {
        Effekseer::Matrix44 viewMat = mEffectPass->ToEfkMatrix(ctx.camera->GetViewMatrix());
        Effekseer::Matrix44 projMat = mEffectPass->ToEfkMatrix(ctx.camera->GetProjectionMatrix());
        mEffectPass->Execute(ctx.deltaTime, viewMat, projMat);

        // Effekseer가 변경한 RootSignature/DescriptorHeap 복원
        RENDERMANAGER.SetGraphicsTable();
    }

    mPostProcessPass->Execute(*ctx.deferredBatchs);
}

void LobbyRenderPipeline::RenderDeferred(const RenderContext& ctx)
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
}