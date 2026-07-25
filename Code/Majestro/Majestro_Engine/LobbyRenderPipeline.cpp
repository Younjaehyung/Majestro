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
#include "DualKawaseBlurPass.h"
#include "LobbyBackgroundPass.h"

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
    mLobbyBackgroundPass = make_shared<LobbyBackgroundPass>();
	

    mEffectPass->Initialize(world);
    mPostProcessPass->Initialize();
    mDepthPrePass->Initialize();
    mShadowPass->Initialize();
    mGBufferPass->Initialize();
    mLightPass->Initialize();
    mForwardPass->Initialize();
    mOutlinePass->Initialize();
    mLobbyBackgroundPass->Initialize(world);

    // Dual Kawase 이미시브 블룸 — GodRay 이후 HDR 체인에 등록
    mEmissiveBloomPass = make_shared<DualKawaseBlurPass>();
    mEmissiveBloomPass->Initialize(4);      
    mEmissiveBloomPass->SetThreshold(1.0f); 
    mEmissiveBloomPass->SetIntensity(0.8f); 
    mPostProcessPass->AddHDRPass(mEmissiveBloomPass);

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

    mLobbyBackgroundPass->SetData(table,
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
    ExecuteIndependentGraphics(ctx);
    ExecuteDependentGraphics(ctx);
}

void LobbyRenderPipeline::ExecuteIndependentGraphics(const RenderContext& ctx)
{
    // These passes do not read Forward Plus compute output and can overlap with it.
    mDepthPrePass->Execute(*ctx.deferredBatchs);
    RenderDeferred(ctx);

    mLobbyBackgroundPass->Execute(*ctx.deferredBatchs);
}

void LobbyRenderPipeline::ExecuteDependentGraphics(const RenderContext& ctx)
{
    // Forward rendering is the first pass that consumes Forward Plus compute output.
    mForwardPass->Execute(*ctx.deferredBatchs);
    mOutlinePass->Execute(*ctx.deferredBatchs);
    // Effekseer VFX
    if (ctx.camera)
    {
        Effekseer::Matrix44 viewMat = mEffectPass->ToEfkMatrix(ctx.camera->GetViewMatrix());
        Effekseer::Matrix44 projMat = mEffectPass->ToEfkMatrix(ctx.camera->GetProjectionMatrix());
        mEffectPass->Execute(ctx.deltaTime, viewMat, projMat, ctx.camera->mNear, ctx.camera->mFar);

        // Effekseer가 변경한 RootSignature/DescriptorHeap 복원
        RENDERMANAGER.SetGraphicsTable();
    }

    mPostProcessPass->Execute(*ctx.deferredBatchs);
}

// ─────────────────────────────────────────────────────────────────────────────
// ImGui 디버그 창 (로비 파이프라인)
// ─────────────────────────────────────────────────────────────────────────────

void LobbyRenderPipeline::DrawImGui()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Render Pipeline"))
    {
        ImGui::End();
        return;
    }

    // ── Pass On/Off ───────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Pass On/Off", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool fogOn = mFogPass ? mFogPass->IsEnabled() : false;
        if (ImGui::Checkbox("Fog", &fogOn) && mFogPass)
            mFogPass->SetEnabled(fogOn);

        bool bloomOn = mEmissiveBloomPass ? mEmissiveBloomPass->IsEnabled() : false;
        if (ImGui::Checkbox("EmissiveBloom", &bloomOn) && mEmissiveBloomPass)
            mEmissiveBloomPass->SetEnabled(bloomOn);

        bool outlineOn = mOutlinePass ? mOutlinePass->IsEnabled() : false;
        if (ImGui::Checkbox("Outline", &outlineOn) && mOutlinePass)
            mOutlinePass->SetEnabled(outlineOn);
    }

    // ── EmissiveBloom 파라미터 ────────────────────────────────────────────────
    if (mEmissiveBloomPass && ImGui::CollapsingHeader("EmissiveBloom 파라미터"))
    {
        float threshold = mEmissiveBloomPass->GetThreshold();
        if (ImGui::SliderFloat("Threshold##EB", &threshold, 0.0f, 3.0f))
            mEmissiveBloomPass->SetThreshold(threshold);

        float intensity = mEmissiveBloomPass->GetIntensity();
        if (ImGui::SliderFloat("Intensity##EB", &intensity, 0.0f, 3.0f))
            mEmissiveBloomPass->SetIntensity(intensity);
    }

    // ── ColorGrading ─────────────────────────────────────────────────────────
    if (mPostProcessPass && ImGui::CollapsingHeader("ColorGrading"))
    {
        ColorGradingParams p = mPostProcessPass->GetColorGrading();
        bool changed = false;

        changed |= ImGui::SliderFloat("Saturation##CG",  &p.Saturation,  0.0f, 3.0f);
        changed |= ImGui::SliderFloat("Contrast##CG",    &p.Contrast,    0.0f, 3.0f);
        changed |= ImGui::SliderFloat("Brightness##CG",  &p.Brightness, -1.0f, 1.0f);
        changed |= ImGui::SliderFloat("Exposure##CG",    &p.Exposure,    0.1f, 5.0f);

        ImGui::Separator();
        float shadow[3] = { p.ShadowTint.x, p.ShadowTint.y, p.ShadowTint.z };
        if (ImGui::ColorEdit3("ShadowTint##CG", shadow))
        {
            p.ShadowTint = Vec3(shadow[0], shadow[1], shadow[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("ShadowStrength##CG",    &p.ShadowStrength,    0.0f, 1.0f);

        float mid[3] = { p.MidtoneTint.x, p.MidtoneTint.y, p.MidtoneTint.z };
        if (ImGui::ColorEdit3("MidtoneTint##CG", mid))
        {
            p.MidtoneTint = Vec3(mid[0], mid[1], mid[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("MidtoneStrength##CG",   &p.MidtoneStrength,   0.0f, 1.0f);

        float hi[3] = { p.HighlightTint.x, p.HighlightTint.y, p.HighlightTint.z };
        if (ImGui::ColorEdit3("HighlightTint##CG", hi))
        {
            p.HighlightTint = Vec3(hi[0], hi[1], hi[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("HighlightStrength##CG", &p.HighlightStrength, 0.0f, 1.0f);

        if (changed)
            mPostProcessPass->SetColorGrading(p);
    }

    ImGui::End();
#endif
}

void LobbyRenderPipeline::RenderDeferred(const RenderContext& ctx)
{

    mGBufferPass->Execute(*ctx.deferredBatchs);

    // Depth 상태 전환: DEPTH_WRITE -> DEPTH_READ | PIXEL_SHADER_RESOURCE
    auto* deferredDepthResource = RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).GetDSTexture()->GetTex2D().Get();
    {
        auto toDepthRead = CD3DX12_RESOURCE_BARRIER::Transition(
            deferredDepthResource,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthRead);
    }

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
    // final_PS가 Gbuffer[0]으로 배경 판별
    hdrGroup.OMSetRenderTargetsReadOnlyDepth();

    RESOURCEMANAGER.Get<Shader>(L"Final")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    if (RENDERMANAGER.IsMsaaEnabled())
    {
        RENDERMANAGER.GetRenderTargetGroup(
            static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN))
            .WaitTargetToResource();
    }
    hdrGroup.WaitTargetToResource();

    // ─── Depth 상태 복구: DEPTH_READ | PIXEL_SHADER_RESOURCE -> DEPTH_WRITE ──────
    {
        auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
            deferredDepthResource,
            D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
    }
}
