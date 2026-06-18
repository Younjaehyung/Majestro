#include "pch.h"
#include "GameRenderPipeline.h"


#include "DepthPrePass.h"
#include "ShadowPass.h"
#include "GBufferPass.h"
#include "LightsPass.h"
#include "ForwardPass.h"
#include "OutlinePass.h"
#include "EffectPass.h"
#include "TrailRenderPass.h"
#include "ParticlePass.h"
#include "RenderPass.h"     
#include "MotionVectorPass.h"
#include "FogPass.h"
#include "MotionBlurPass.h"
#include "LuminancePass.h"
#include "ToneMapPass.h"
#include "GodRayPass.h"
#include "DualKawaseBlurPass.h"
#include "HBAOPass.h"
#include "HealthVignettePass.h"
#include "FXAAPass.h"
#include "WorldUIPass.h"
#include "UIFeature.h"
#include "LightComponent.h"


#include "Engine.h"
#include "RenderManager.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "World.h"
#include "CameraComponent.h"
#include "PlayerComponent.h" 
#include "HealthComponent.h"
#include "RenderSystem.h"
#include "TagComponent.h"
#include "InputManager.h"

void GameRenderPipeline::Initialize(World* world)
{
    mWorld = world;


    mDepthPrePass    = make_shared<DepthPrePass>();
    mShadowPass      = make_shared<ShadowPass>();
    mGBufferPass     = make_shared<GBufferPass>();
    mLightPass       = make_shared<LightsPass>();
    mMotionVectorPass = make_shared<MotionVectorPass>();
    mForwardPass     = make_shared<ForwardPass>();
    mOutlinePass     = make_shared<OutlinePass>();
    mEffectPass      = make_shared<EffectPass>();
    mTrailRenderPass = make_shared<TrailRenderPass>();
    mParticlePass    = make_shared<ParticlePass>();
    mPostProcessPass = make_shared<PostProcessPass>();
    mWorldUIPass    = make_shared<WorldUIPass>();

    mHBAOPass = make_shared<HBAOPass>();
    mHBAOPass->Initialize();
    mHBAOPass->SetRadius(50.f);
    mHBAOPass->SetBias(0.1f);       // 자기-차폐 방지 (값 높을수록 AO 줄어듦)
    mHBAOPass->SetIntensity(1.2f);  // 전반적 강도
    mHBAOPass->SetFalloff(2.0f);
    mHBAOPass->SetNumDirections(2);
    mHBAOPass->SetNumSteps(2);
    mHBAOPass->SetBlurRadius(3.0f);

    mDepthPrePass->Initialize();
	mShadowPass->Initialize();
	mGBufferPass->Initialize();
	mLightPass->Initialize();
    mMotionVectorPass->Initialize();
    mForwardPass->Initialize();
    mOutlinePass->Initialize();
    mEffectPass->Initialize(world);
    mTrailRenderPass->Initialize(world);
    mParticlePass->Initialize(world);
    mPostProcessPass->Initialize();
    mWorldUIPass->Initialize(world);

    
   
    mMotionBlurPass = make_shared<MotionBlurPass>();
    mPostProcessPass->AddHDRPass(mMotionBlurPass);

    mFogPass = make_shared<FogPass>();
    mPostProcessPass->AddHDRPass(mFogPass);

    // mLuminancePass = make_shared<LuminancePass>();
    // mPostProcessPass->AddLDRPass(mLuminancePass);

    mGodRayPass = make_shared<GodRayPass>();
    mGodRayPass->SetIntensity(1.75f);
    mGodRayPass->SetNumSteps(8);
    mGodRayPass->SetMaxRayLen(8000.0f);
    mGodRayPass->SetScatterCoeff(0.00008f);
    mGodRayPass->SetMieAsymmetry(0.76f);
    mGodRayPass->SetSunColor(Vec3(1.0f, 0.97f, 0.82f));
    mGodRayPass->SetAbsorptionCoeff(0.00002f);
    mPostProcessPass->AddHDRPass(mGodRayPass);

    mEmissiveBloomPass = make_shared<DualKawaseBlurPass>();
    mEmissiveBloomPass->Initialize(3);      // 4단계 (W/2 ->W/4 -> W/8 -> W/16)
    mEmissiveBloomPass->SetThreshold(1.0f); // LDR 범위 초과 밝기부터 추출
    mEmissiveBloomPass->SetIntensity(0.8f); // 최종 합성 강도
    mPostProcessPass->AddHDRPass(mEmissiveBloomPass);

    // 체력 회복과 빈사 상태의 화면 외곽 비네팅
    mHealthVignettePass = make_shared<HealthVignettePass>();
    mHealthVignettePass->Initialize();
    mPostProcessPass->AddLDRPass(mHealthVignettePass);

    // FXAA:
    mFXAAPass = make_shared<FXAAPass>();
    mFXAAPass->Initialize();
    mFXAAPass->SetParams(
        0.125f,  // edgeThreshold    : 루마 대비 감지 임계값 (낮을수록 더 많은 엣지 처리)
        0.0625f, // edgeThresholdMin : 어두운 영역 컷오프 (매우 어두운 픽셀 처리 생략)
        0.75f    // subpixQuality    : 서브픽셀 블렌드 강도 (0=꺼짐, 1=최대)
    );
    mPostProcessPass->AddLDRPass(mFXAAPass);

    // 사용자 그래픽 설정 복원
    ApplyGraphicsSettings();
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

    mHBAOPass->SetData(table,
        RENDER_TARGET_GROUP_TYPE::G_BUFFER,
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

    // GodRay: 매 프레임 DirectionalLight 방향을 읽어 갱신
    if (mGodRayPass && mWorld && mWorld->HasComponentPool<LightComponent>())
    {
        auto lights = mWorld->GetEntitiesWithComponent<LightComponent>();
        for (auto e : lights)
        {
            auto* light = mWorld->GetComponent<LightComponent>(e);
            if (light && light->GetLightType() == LIGHT_TYPE::DIRECTIONAL_LIGHT)
            {
                Vec3 dir = Vec3(light->mLightInfo.Direction.x,
                                light->mLightInfo.Direction.y,
                                light->mLightInfo.Direction.z);
                mGodRayPass->SetSunDirection(dir);
                break;
            }
        }
    }

    UpdateHealthVignetteState();

    mPostProcessPass->SetData(table);
}

void GameRenderPipeline::PreCompute(const RenderContext& ctx)
{
    mForwardPass->Compute();
}


void GameRenderPipeline::Execute(const RenderContext& ctx)
{
    ExecuteIndependentGraphics(ctx);
    ExecuteDependentGraphics(ctx);
}

void GameRenderPipeline::ExecuteIndependentGraphics(const RenderContext& ctx)
{
    UpdatePassStates();

    // These passes do not read Forward Plus compute output and can overlap with it.
    RenderDepthPrePass(ctx);
    RenderShadow(ctx);
    RenderDeferred(ctx);
}

void GameRenderPipeline::ExecuteDependentGraphics(const RenderContext& ctx)
{
    // Forward rendering is the first pass that consumes Forward Plus compute output.
    RenderForward(ctx);
    RenderOutline(ctx);
    RenderEffect(ctx);
    RenderPost(ctx);
    RenderWorldUI(ctx);

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
            enableBlur = (player->mLowerState == static_cast<int>(ReplicatedMovementMode::Dashing));
        }
        break;
    }
    mMotionBlurPass->SetEnabled(enableBlur);
}

void GameRenderPipeline::UpdateHealthVignetteState()
{
    if (!mHealthVignettePass)
        return;

    const float dt = TIMER.GetTimeElapsed();

    const auto applyFadeOut = [&]()
        {

            mHasPreviousHealthRatio = false;
            const float alpha = std::clamp(dt * 8.0f, 0.0f, 1.0f);
            mHealthVignetteLowStrength += (0.0f - mHealthVignetteLowStrength) * alpha;
            mHealthVignetteHealTimer = (std::max)(0.0f, mHealthVignetteHealTimer - dt);

            const float healStrength = (mHealthVignetteHealDuration > 0.0f)
                ? std::clamp(mHealthVignetteHealTimer / mHealthVignetteHealDuration, 0.0f, 1.0f)
                : 0.0f;
            mHealthVignettePass->SetFeedbackState(mHealthVignetteLowStrength, healStrength);
        };

    if (mWorld == nullptr || mWorld->HasComponentPool<LocalPlayerComponent>() == false)
    {
        applyFadeOut();
        return;
    }

    float hpRatio = 1.0f;
    bool hasHealth = false;

    auto players = mWorld->GetEntitiesWithComponent<LocalPlayerComponent>();
    for (auto e : players)
    {
        HealthComponent* health = mWorld->GetComponent<HealthComponent>(e);
        if (health == nullptr || health->mMaxHp <= 0)
            continue;

        hpRatio = std::clamp(
            static_cast<float>(health->mCurrentHp) / static_cast<float>((std::max)(1, health->mMaxHp)),
            0.0f,
            1.0f);
        hasHealth = true;
        break;
    }

    if (hasHealth == false)
    {
        applyFadeOut();
        return;
    }

    if (mHasPreviousHealthRatio == false)
    {
        mHasPreviousHealthRatio = true;
        mPreviousHealthRatio = hpRatio;
    }
    else if (hpRatio > mPreviousHealthRatio + 0.001f)
    {
        mHealthVignetteHealTimer = mHealthVignetteHealDuration;
        mPreviousHealthRatio = hpRatio;
    }
    else if (hpRatio < mPreviousHealthRatio - 0.001f)
    {
        mPreviousHealthRatio = hpRatio;
    }

    const float rawLowTarget = (hpRatio < mHealthVignetteLowThreshold)
        ? 1.0f - (hpRatio / mHealthVignetteLowThreshold)
        : 0.0f;
    const float lowTarget = std::clamp(rawLowTarget * 1.35f, 0.0f, 1.0f);

    const float lowAlpha = std::clamp(dt * 7.0f, 0.0f, 1.0f);
    mHealthVignetteLowStrength += (lowTarget - mHealthVignetteLowStrength) * lowAlpha;
    mHealthVignetteHealTimer = (std::max)(0.0f, mHealthVignetteHealTimer - dt);

    const float healStrength = (mHealthVignetteHealDuration > 0.0f)
        ? std::clamp(mHealthVignetteHealTimer / mHealthVignetteHealDuration, 0.0f, 1.0f)
        : 0.0f;
    mHealthVignettePass->SetFeedbackState(mHealthVignetteLowStrength, healStrength);
}

// ─────────────────────────────────────────────────────────────────────────────
// 제어 API
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderPipeline::SetMotionBlurEnabled(bool on)
{
    if (mMotionBlurPass) mMotionBlurPass->SetEnabled(on);
    if (mMotionVectorPass) mMotionVectorPass->SetEnabled(on);
}

void GameRenderPipeline::SetFogEnabled(bool on)
{
    if (mFogPass) mFogPass->SetEnabled(on);
}

void GameRenderPipeline::SetGodRayEnabled(bool on)
{
    if (mGodRayPass) mGodRayPass->SetEnabled(on);
}

void GameRenderPipeline::SetOutlineEnabled(bool on)
{
    if (mOutlinePass) mOutlinePass->SetEnabled(on);
}

void GameRenderPipeline::SetEmissiveBloomEnabled(bool on)
{
    if (mEmissiveBloomPass) mEmissiveBloomPass->SetEnabled(on);
}

void GameRenderPipeline::SetHBAOEnabled(bool on)
{
    if (mHBAOPass) mHBAOPass->SetEnabled(on);
}

void GameRenderPipeline::SetFXAAEnabled(bool on)
{
    if (mFXAAPass) mFXAAPass->SetEnabled(on);
}

// Pass on/off 조회
bool GameRenderPipeline::IsFogEnabled()     const { return mFogPass          && mFogPass->IsEnabled(); }
bool GameRenderPipeline::IsGodRayEnabled()  const { return mGodRayPass       && mGodRayPass->IsEnabled(); }
bool GameRenderPipeline::IsBloomEnabled()   const { return mEmissiveBloomPass && mEmissiveBloomPass->IsEnabled(); }
bool GameRenderPipeline::IsOutlineEnabled() const { return mOutlinePass      && mOutlinePass->IsEnabled(); }
bool GameRenderPipeline::IsHBAOEnabled()    const { return mHBAOPass         && mHBAOPass->IsEnabled(); }
bool GameRenderPipeline::IsFXAAEnabled()    const { return mFXAAPass         && mFXAAPass->IsEnabled(); }

void GameRenderPipeline::ApplyGraphicsSettings()
{
    const GraphicsSettings& g = RENDERMANAGER.GetGraphicsSettings();
    SetFogEnabled(g.bFog);
    SetGodRayEnabled(g.bGodRay);
    SetEmissiveBloomEnabled(g.bBloom);
    SetOutlineEnabled(g.bOutline);
    SetHBAOEnabled(g.bHBAO);
    SetFXAAEnabled(g.bFXAA);
}

void GameRenderPipeline::SetFXAAParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality)
{
    if (mFXAAPass) mFXAAPass->SetParams(edgeThreshold, edgeThresholdMin, subpixQuality);
}

void GameRenderPipeline::SetHealthVignetteNoiseTexture(const std::wstring& textureName)
{
    if (mHealthVignettePass)
        mHealthVignettePass->SetNoiseTextureName(textureName);
}

void GameRenderPipeline::SetWorldUIFeature(std::vector<shared_ptr<UIFeature>>* features)
{
    if (mWorldUIPass)
		mWorldUIPass->SetFeatures(features);
}


void GameRenderPipeline::SetColorLUT(const std::wstring& name, int size, float strength)
{
    if (mPostProcessPass) mPostProcessPass->SetColorLUT(name, size, strength);
}

void GameRenderPipeline::SetBlur(bool on)
{
    mIsBlured = on;
    if (mPostProcessPass) mPostProcessPass->SetBlur(mIsBlured);
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
    static bool sDummyDirty = false;  // ctx 미연결 파이프라인 폴백
    mShadowPass->Execute(
        *ctx.cascadeBatchs,
        const_cast<std::array<bool, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>&>(*ctx.cascadeActive),
        ctx.mapCascadeDirty ? *ctx.mapCascadeDirty : sDummyDirty);
}

void GameRenderPipeline::RenderDeferred(const RenderContext& ctx)
{

    mGBufferPass->Execute(*ctx.deferredBatchs);

    // HBAO+: G-Buffer 완성 직후, 조명 계산 전에 AO 생성
    // (Gbuffer[1]=Position, Gbuffer[2]=Normal 이 PSR 상태로 준비되어 있음)
    mHBAOPass->Execute(*ctx.deferredBatchs);

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
    mEffectPass->Execute(dt, viewMat, projMat, ctx.camera->mNear, ctx.camera->mFar);
    // TrailRender change: weapon and dash ribbon VFX are batched through one HDR trail pass.
    mTrailRenderPass->Execute(ctx);
    mParticlePass->Execute(ctx);


    // Effekseer가 RootSignature/DescriptorHeap을 변경하므로 엔진 상태 복원
    RENDERMANAGER.SetGraphicsTable();
}

void GameRenderPipeline::RenderPost(const RenderContext& ctx)
{
    mPostProcessPass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderWorldUI(const RenderContext& ctx)
{
    if (mWorldUIPass)
        mWorldUIPass->Execute(ctx.camera);
}

// ─────────────────────────────────────────────────────────────────────────────
// ImGui 디버그 창 (게임 파이프라인)
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderPipeline::DrawImGui()
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
        if (ImGui::Checkbox("Fog",              &fogOn))          SetFogEnabled(fogOn);

        bool godRayOn = mGodRayPass ? mGodRayPass->IsEnabled() : false;
        if (ImGui::Checkbox("GodRay (VLS)",     &godRayOn))       SetGodRayEnabled(godRayOn);

        bool bloomOn = mEmissiveBloomPass ? mEmissiveBloomPass->IsEnabled() : false;
        if (ImGui::Checkbox("EmissiveBloom",    &bloomOn))        SetEmissiveBloomEnabled(bloomOn);

        bool outlineOn = mOutlinePass ? mOutlinePass->IsEnabled() : false;
        if (ImGui::Checkbox("Outline",          &outlineOn))      SetOutlineEnabled(outlineOn);

        bool hbaoOn = mHBAOPass ? mHBAOPass->IsEnabled() : false;
        if (ImGui::Checkbox("HBAO+",            &hbaoOn))         SetHBAOEnabled(hbaoOn);

        bool fxaaOn = mFXAAPass ? mFXAAPass->IsEnabled() : false;
        if (ImGui::Checkbox("FXAA",             &fxaaOn))         SetFXAAEnabled(fxaaOn);

        bool collidersOn = RenderSystem::GetDrawColliders();
        if (ImGui::Checkbox("Collision Boxes", &collidersOn))
            RenderSystem::SetDrawColliders(collidersOn);

        bool cullingObbOn = RenderSystem::GetDrawCullingOBB();
        if (ImGui::Checkbox("Culling OBB (mWorldOBB)", &cullingObbOn))
            RenderSystem::SetDrawCullingOBB(cullingObbOn);

        bool enemyRangesOn = RenderSystem::GetDrawEnemyRanges();
        if (ImGui::Checkbox("Enemy Range Circles", &enemyRangesOn))
            RenderSystem::SetDrawEnemyRanges(enemyRangesOn);

        bool enemyAttackRangesOn = RenderSystem::GetDrawEnemyAttackRanges();
        if (ImGui::Checkbox("Enemy Attack Ranges", &enemyAttackRangesOn))
            RenderSystem::SetDrawEnemyAttackRanges(enemyAttackRangesOn);

        bool playerAttackRangesOn = RenderSystem::GetDrawPlayerAttackRanges();
        if (ImGui::Checkbox("Player Attack Ranges", &playerAttackRangesOn))
            RenderSystem::SetDrawPlayerAttackRanges(playerAttackRangesOn);

        bool remoteDesktopMouseMode =
            (INPUT.GetMouseInputMode() == eMouseInputMode::LegacyRelative);
        if (ImGui::Checkbox("Remote Desktop Mouse Mode", &remoteDesktopMouseMode))
        {
            INPUT.SetMouseInputMode(
                remoteDesktopMouseMode
                ? eMouseInputMode::LegacyRelative
                : eMouseInputMode::RecenterRelative);
        }

        // MotionBlur는 대시 중 자동 활성화 — 수동 오버라이드만 허용
        ImGui::BeginDisabled(true);
        bool blurOn = mMotionBlurPass ? mMotionBlurPass->IsEnabled() : false;
        ImGui::Checkbox("MotionBlur (auto: Dash)", &blurOn);
        ImGui::EndDisabled();
    }

    // ── GodRay 파라미터 ───────────────────────────────────────────────────────
    if (mGodRayPass && ImGui::CollapsingHeader("GodRay 파라미터"))
    {
        float intensity = mGodRayPass->GetIntensity();
        if (ImGui::SliderFloat("Intensity##GR", &intensity, 0.0f, 10.0f))
            mGodRayPass->SetIntensity(intensity);

        int steps = mGodRayPass->GetNumSteps();
        if (ImGui::SliderInt("NumSteps##GR", &steps, 8, 128))
            mGodRayPass->SetNumSteps(steps);

        float maxRayLen = mGodRayPass->GetMaxRayLen();
        if (ImGui::DragFloat("MaxRayLen##GR", &maxRayLen, 100.0f, 100.0f, 20000.0f))
            mGodRayPass->SetMaxRayLen(maxRayLen);

        float scatter = mGodRayPass->GetScatterCoeff();
        if (ImGui::SliderFloat("ScatterCoeff##GR", &scatter, 0.00001f, 0.001f, "%.6f"))
            mGodRayPass->SetScatterCoeff(scatter);

        float mie = mGodRayPass->GetMieAsymmetry();
        if (ImGui::SliderFloat("MieAsymmetry##GR", &mie, -1.0f, 1.0f))
            mGodRayPass->SetMieAsymmetry(mie);

        float absorption = mGodRayPass->GetAbsorptionCoeff();
        if (ImGui::SliderFloat("AbsorptionCoeff##GR", &absorption, 0.0f, 0.001f, "%.6f"))
            mGodRayPass->SetAbsorptionCoeff(absorption);

        const Vec3& sc = mGodRayPass->GetSunColor();
        float col[3] = { sc.x, sc.y, sc.z };
        if (ImGui::ColorEdit3("SunColor##GR", col))
            mGodRayPass->SetSunColor(Vec3(col[0], col[1], col[2]));
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

    // ── HBAO+ 파라미터 ────────────────────────────────────────────────────────
    if (mHBAOPass && ImGui::CollapsingHeader("HBAO+ 파라미터"))
    {
        float radius = mHBAOPass->GetRadius();
        if (ImGui::SliderFloat("Radius##HBAO", &radius, 0.05f, 5.0f))
            mHBAOPass->SetRadius(radius);

        float bias = mHBAOPass->GetBias();
        if (ImGui::SliderFloat("Bias##HBAO", &bias, 0.0f, 0.2f))
            mHBAOPass->SetBias(bias);

        float hbaoIntensity = mHBAOPass->GetIntensity();
        if (ImGui::SliderFloat("Intensity##HBAO", &hbaoIntensity, 0.0f, 5.0f))
            mHBAOPass->SetIntensity(hbaoIntensity);

        float falloff = mHBAOPass->GetFalloff();
        if (ImGui::SliderFloat("Falloff##HBAO", &falloff, 0.1f, 5.0f))
            mHBAOPass->SetFalloff(falloff);

        int numDirs = mHBAOPass->GetNumDirections();
        if (ImGui::SliderInt("NumDirections##HBAO", &numDirs, 2, 16))
            mHBAOPass->SetNumDirections(numDirs);

        int numSteps = mHBAOPass->GetNumSteps();
        if (ImGui::SliderInt("NumSteps##HBAO", &numSteps, 2, 16))
            mHBAOPass->SetNumSteps(numSteps);

        float blurRad = mHBAOPass->GetBlurRadius();
        if (ImGui::SliderFloat("BlurRadius##HBAO", &blurRad, 0.0f, 10.0f))
            mHBAOPass->SetBlurRadius(blurRad);
    }

    // ── FXAA 파라미터 ─────────────────────────────────────────────────────────
    if (mFXAAPass && ImGui::CollapsingHeader("FXAA 파라미터"))
    {
        float edgeThr    = mFXAAPass->GetEdgeThreshold();
        float edgeThrMin = mFXAAPass->GetEdgeThresholdMin();
        float subpix     = mFXAAPass->GetSubpixQuality();

        bool changed = false;
        changed |= ImGui::SliderFloat("EdgeThreshold##FXAA",    &edgeThr,    0.063f, 0.333f);
        changed |= ImGui::SliderFloat("EdgeThresholdMin##FXAA", &edgeThrMin, 0.031f, 0.083f);
        changed |= ImGui::SliderFloat("SubpixQuality##FXAA",    &subpix,     0.0f,   1.0f);

        if (changed)
            mFXAAPass->SetParams(edgeThr, edgeThrMin, subpix);
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
