#include "pch.h"
#include "GameRenderPipeline.h"


#include "DepthPrePass.h"
#include "ShadowPass.h"
#include "GBufferPass.h"
#include "LightsPass.h"
#include "ForwardPass.h"
#include "OutlinePass.h"
#include "DecalPass.h"
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
#include "PlayerStatusComponent.h"
#include "HealthComponent.h"
#include "RenderSystem.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "EngineLog.h"

namespace
{
    constexpr wchar_t kColorLUTDirectory[] = L"..\\Resources\\Texture\\LUTs";

    enum class PipelineProfilePass : uint32
    {
        Depth = 0,
        Shadow,
        Deferred,
        Decal,
        Forward,
        Outline,
        Effect,
        Post,
        WorldUI,
        Count
    };

    struct PipelineCpuProfile
    {
        uint32 frameCount = 0;
        array<
            EngineLog::CpuProfileSample,
            static_cast<uint32>(PipelineProfilePass::Count)> samples{};

        void Reset()
        {
            *this = {};
        }

        EngineLog::CpuProfileSample& Get(PipelineProfilePass pass)
        {
            return samples[static_cast<uint32>(pass)];
        }
    };

    PipelineCpuProfile sPipelineCpuProfile;

    // 각 렌더 패스의 CPU 명령 기록 시간을 평균과 최대값으로 함께 수집한다.
    void OutputPipelineCpuProfile()
    {
        if (!EngineLog::Enabled(EngineLog::Domain::RenderPassPerformance))
            return;

        if (++sPipelineCpuProfile.frameCount < 60)
            return;

        const uint32 frameCount = sPipelineCpuProfile.frameCount;
        wchar_t buffer[1024]{};
        swprintf_s(
            buffer,
            L"[PASS_CPU] avg/max ms | Depth %.2f/%.2f | Shadow %.2f/%.2f | "
            L"Deferred %.2f/%.2f | Decal %.2f/%.2f | Forward %.2f/%.2f | "
            L"Outline %.2f/%.2f | Effect %.2f/%.2f | Post %.2f/%.2f | "
            L"WorldUI %.2f/%.2f\n",
            sPipelineCpuProfile.samples[0].Average(frameCount),
            sPipelineCpuProfile.samples[0].maxMs,
            sPipelineCpuProfile.samples[1].Average(frameCount),
            sPipelineCpuProfile.samples[1].maxMs,
            sPipelineCpuProfile.samples[2].Average(frameCount),
            sPipelineCpuProfile.samples[2].maxMs,
            sPipelineCpuProfile.samples[3].Average(frameCount),
            sPipelineCpuProfile.samples[3].maxMs,
            sPipelineCpuProfile.samples[4].Average(frameCount),
            sPipelineCpuProfile.samples[4].maxMs,
            sPipelineCpuProfile.samples[5].Average(frameCount),
            sPipelineCpuProfile.samples[5].maxMs,
            sPipelineCpuProfile.samples[6].Average(frameCount),
            sPipelineCpuProfile.samples[6].maxMs,
            sPipelineCpuProfile.samples[7].Average(frameCount),
            sPipelineCpuProfile.samples[7].maxMs,
            sPipelineCpuProfile.samples[8].Average(frameCount),
            sPipelineCpuProfile.samples[8].maxMs);
        EngineLog::OutputPerformance(
            EngineLog::Domain::RenderPassPerformance,
            buffer);
        sPipelineCpuProfile.Reset();
    }

    bool IsColorLUTExtension(const std::filesystem::path& path)
    {
        const wstring extension = path.extension().wstring();
        return extension == L".png" || extension == L".PNG" ||
               extension == L".dds" || extension == L".DDS";
    }

    bool IsDDS(const std::filesystem::path& path)
    {
        const wstring extension = path.extension().wstring();
        return extension == L".dds" || extension == L".DDS";
    }

    int GetColorLUTOrder(const wstring& name)
    {
        size_t firstDigit = name.find_first_of(L"0123456789");
        if (firstDigit == wstring::npos)
            return 2147483647;

        return _wtoi(name.c_str() + firstDigit);
    }

    int GetLogicalColorLUTSize(int width, int height)
    {
        if (width <= 0 || height <= 0 || width % height != 0)
            return 0;

        const int logicalSize = width / height;
        if (logicalSize < 2 || height % logicalSize != 0)
            return 0;

        return logicalSize;
    }

}

void GameRenderPipeline::Initialize(World* world)
{
    mWorld = world;
    sPipelineCpuProfile.Reset();


    mDepthPrePass = make_shared<DepthPrePass>();
    mShadowPass = make_shared<ShadowPass>();
    mGBufferPass = make_shared<GBufferPass>();
    mLightPass = make_shared<LightsPass>();
    mMotionVectorPass = make_shared<MotionVectorPass>();
    mForwardPass = make_shared<ForwardPass>();
    mOutlinePass = make_shared<OutlinePass>();
    mDecalPass = make_shared<DecalPass>();
    mEffectPass = make_shared<EffectPass>();
    mTrailRenderPass = make_shared<TrailRenderPass>();
    mParticlePass = make_shared<ParticlePass>();
    mPostProcessPass = make_shared<PostProcessPass>();
    mWorldUIPass = make_shared<WorldUIPass>();

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
    mDecalPass->Initialize(world);
    mEffectPass->Initialize(world);
    mTrailRenderPass->Initialize(world);
    mParticlePass->Initialize(world);
    mPostProcessPass->Initialize();
    mWorldUIPass->Initialize(world);
    DiscoverColorLUTs();



    mMotionBlurPass = make_shared<MotionBlurPass>();
    mMotionBlurPass ->Initialize();
    mPostProcessPass->AddHDRPass(mMotionBlurPass);

    mFogPass = make_shared<FogPass>();
    mFogPass->Initialize();
    mPostProcessPass->AddHDRPass(mFogPass);

    // mLuminancePass = make_shared<LuminancePass>();
    // mPostProcessPass->AddLDRPass(mLuminancePass);

    mGodRayPass = make_shared<GodRayPass>();
    mGodRayPass->SetIntensity(2.1f);
    mGodRayPass->SetNumSteps(12);   // 레이마칭 스텝 상향 — 밴딩 감소 / 부드러운 산란
    mGodRayPass->SetMaxRayLen(8000.0f);
    mGodRayPass->SetScatterCoeff(0.00008f);
    mGodRayPass->SetMieAsymmetry(0.76f);
    mGodRayPass->SetSunColor(Vec3(1.0f, 0.97f, 0.82f));
    mGodRayPass->SetAbsorptionCoeff(0.00002f);
    mGodRayPass->Initialize();   // 절반 해상도 scatter 타깃 생성
    mPostProcessPass->AddHDRPass(mGodRayPass);

    mEmissiveBloomPass = make_shared<DualKawaseBlurPass>();
    mEmissiveBloomPass->Initialize(2);      // 4단계 (W/2 ->W/4 -> W/8 -> W/16)
    mEmissiveBloomPass->SetThreshold(1.0f); // LDR 범위 초과 밝기부터 추출
    mEmissiveBloomPass->SetIntensity(1.15f); // 최종 합성 강도
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

    // MotionBlur && MotionVector
    UpdatePassStates();
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

    RenderDepthPrePass(ctx);
    RenderShadow(ctx);
    RenderDeferred(ctx);
}

void GameRenderPipeline::ExecuteDependentGraphics(const RenderContext& ctx)
{
    RenderDecal(ctx);
    RenderForward(ctx);
    RenderOutline(ctx);
    RenderEffect(ctx);
    RenderPost(ctx);
    RenderWorldUI(ctx);

    // 파이프라인 실행 인터페이스를 오염시키지 않고 패스 계측 주기를 여기서 마감한다.
    OutputPipelineCpuProfile();
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderPipeline::UpdatePassStates()
{
    if (!mMotionBlurPass && !mMotionVectorPass)
        return;

    bool enableBlur = false;
    if (mWorld && mWorld->HasComponentPool<LocalPlayerComponent>())
    {
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
    }

    // MotionBlur , MotionVector
    if (mMotionBlurPass)   mMotionBlurPass->SetEnabled(enableBlur);
    if (mMotionVectorPass) mMotionVectorPass->SetEnabled(enableBlur);
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
            const float buffFadeAlpha = std::clamp(dt * 6.0f, 0.0f, 1.0f);
            mBuffIconStrengths += (Vec4::Zero - mBuffIconStrengths) * buffFadeAlpha;
            mHealthVignettePass->SetFeedbackState(
                mHealthVignetteLowStrength,
                healStrength,
                mBuffIconStrengths);
        };

    if (mWorld == nullptr || mWorld->HasComponentPool<LocalPlayerComponent>() == false)
    {
        applyFadeOut();
        return;
    }

    float hpRatio = 1.0f;
    bool hasHealth = false;
    PlayerStatusComponent* localStatus = nullptr;

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
        localStatus = mWorld->GetComponent<PlayerStatusComponent>(e);
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
    Vec4 targetBuffIconStrengths = Vec4::Zero;
    if (localStatus != nullptr)
    {
        // 회복 효과는 더하기, 이동 효과는 화살표, 
        // 보호 효과는 방패, 공격 효과는 검 아이콘으로 화면 외곽
        const bool hasHeal =
            localStatus->FindBuff(ReplicatedBuffType::HealOverTime) != nullptr;
        const bool hasMoveSpeed =
            localStatus->FindBuff(ReplicatedBuffType::MoveSpeedUp) != nullptr;
        const bool hasShield =
            localStatus->FindBuff(ReplicatedBuffType::ShieldOverTime) != nullptr;
        const bool hasAttack =
            localStatus->FindBuff(ReplicatedBuffType::AttackUp) != nullptr ||
            localStatus->FindBuff(ReplicatedBuffType::BuffPowerUp) != nullptr;

        targetBuffIconStrengths = Vec4(
            hasHeal ? 1.0f : 0.0f,
            hasMoveSpeed ? 1.0f : 0.0f,
            hasShield ? 1.0f : 0.0f,
            hasAttack ? 1.0f : 0.0f);
    }

    // 버프가 갱신될 때 아이콘이 갑자기 나타나거나 사라지지 않도록 보간
    const float buffFadeAlpha = std::clamp(dt * 6.0f, 0.0f, 1.0f);
    mBuffIconStrengths += (targetBuffIconStrengths - mBuffIconStrengths) * buffFadeAlpha;

    // 실제 체력이 회복된 순간에도 짧게 회복 아이콘을 표시
    Vec4 displayedBuffIconStrengths = mBuffIconStrengths;
    displayedBuffIconStrengths.x = (std::max)(displayedBuffIconStrengths.x, healStrength);

    mHealthVignettePass->SetFeedbackState(
        mHealthVignetteLowStrength,
        healStrength,
        displayedBuffIconStrengths);
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
bool GameRenderPipeline::IsFogEnabled()     const { return mFogPass && mFogPass->IsEnabled(); }
bool GameRenderPipeline::IsGodRayEnabled()  const { return mGodRayPass && mGodRayPass->IsEnabled(); }
bool GameRenderPipeline::IsBloomEnabled()   const { return mEmissiveBloomPass && mEmissiveBloomPass->IsEnabled(); }
bool GameRenderPipeline::IsOutlineEnabled() const { return mOutlinePass && mOutlinePass->IsEnabled(); }
bool GameRenderPipeline::IsHBAOEnabled()    const { return mHBAOPass && mHBAOPass->IsEnabled(); }
bool GameRenderPipeline::IsFXAAEnabled()    const { return mFXAAPass && mFXAAPass->IsEnabled(); }
bool GameRenderPipeline::IsColorLUTEnabled() const { return mColorLUTEnabled; }
bool GameRenderPipeline::IsColorGradingEnabled() const { return mPostProcessPass ? mPostProcessPass->IsColorGradingEnabled() : false; }


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
    if (!mPostProcessPass)
        return;

    if (name.empty())
    {
        mColorLUTEnabled = false;
        mPostProcessPass->SetColorLUT(L"", 0, 0.0f);
        return;
    }

    for (int index = 0; index < static_cast<int>(mColorLUTEntries.size()); ++index)
    {
        if (mColorLUTEntries[index].ResourceKey == name)
        {
            mColorLUTIndex = index;
            mColorLUTStrength = strength;
            mColorLUTEnabled = true;
            ApplyCurrentColorLUT();
            return;
        }
    }

   
    if (name == L"ColorLUT" && !mColorLUTEntries.empty())
    {
        mColorLUTStrength = strength;
        mColorLUTEnabled = true;
        SelectColorLUT(0);
        return;
    }

    mPostProcessPass->SetColorLUT(name, size, strength);
}

void GameRenderPipeline::SetColorLUTEnabled(bool enabled)
{
  
    mColorLUTEnabled = enabled;
    ApplyCurrentColorLUT();
}

void GameRenderPipeline::DiscoverColorLUTs()
{
    const std::filesystem::path directory(kColorLUTDirectory);
    std::map<wstring, std::filesystem::path> lutPaths;

    if (!std::filesystem::exists(directory))
        return;

    
    for (const auto& file : std::filesystem::recursive_directory_iterator(directory))
    {
        if (!file.is_regular_file() || !IsColorLUTExtension(file.path()))
            continue;

        std::filesystem::path relativePath = file.path().lexically_relative(directory);
        if (relativePath.empty())
            continue;

        const wstring rootDirectory = (*relativePath.begin()).wstring();

        if (rootDirectory == L"Game" || rootDirectory == L"HaldCLUT")
            continue;

        relativePath.replace_extension();
        const wstring name = relativePath.generic_wstring();
        auto existing = lutPaths.find(name);
        if (existing == lutPaths.end() || IsDDS(file.path()))
            lutPaths[name] = file.path();
    }

    wstring selectedKey;
    if (mColorLUTIndex >= 0 && mColorLUTIndex < static_cast<int>(mColorLUTEntries.size()))
        selectedKey = mColorLUTEntries[mColorLUTIndex].ResourceKey;

    mColorLUTEntries.clear();
    for (const auto& [name, path] : lutPaths)
    {
        ColorLUTEntry entry;
        entry.DisplayName = name;
        entry.ResourceKey = L"ColorLUT/" + name;
        entry.Path = path.wstring();
        mColorLUTEntries.push_back(std::move(entry));
    }

    std::sort(mColorLUTEntries.begin(), mColorLUTEntries.end(),
        [](const ColorLUTEntry& left, const ColorLUTEntry& right)
        {
            const int leftOrder = GetColorLUTOrder(left.DisplayName);
            const int rightOrder = GetColorLUTOrder(right.DisplayName);
            if (leftOrder != rightOrder)
                return leftOrder < rightOrder;
            return left.DisplayName < right.DisplayName;
        });

    mColorLUTIndex = -1;
    for (int index = 0; index < static_cast<int>(mColorLUTEntries.size()); ++index)
    {
        if (mColorLUTEntries[index].ResourceKey == selectedKey)
        {
            mColorLUTIndex = index;
            break;
        }
    }

    if (mColorLUTIndex < 0 && !mColorLUTEntries.empty())
        mColorLUTIndex = 0;

    if (mColorLUTIndex >= 0 && mColorLUTEnabled)
        ApplyCurrentColorLUT();
}

void GameRenderPipeline::ApplyCurrentColorLUT()
{
    if (!mPostProcessPass || !mColorLUTEnabled ||
        mColorLUTIndex < 0 || mColorLUTIndex >= static_cast<int>(mColorLUTEntries.size()))
    {
        if (mPostProcessPass)
            mPostProcessPass->SetColorLUT(L"", 0, 0.0f);
        return;
    }

    const ColorLUTEntry& entry = mColorLUTEntries[mColorLUTIndex];
    shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(entry.ResourceKey, entry.Path);
    if (!texture)
        return;

    const int width = static_cast<int>(texture->GetWidth());
    const int height = static_cast<int>(texture->GetHeight());
    const int size = GetLogicalColorLUTSize(width, height);
    if (size <= 1)
    {
        mPostProcessPass->SetColorLUT(L"", 0, 0.0f);
        return;
    }

  
    mPostProcessPass->SetColorLUT(entry.ResourceKey, size, mColorLUTStrength);
}

void GameRenderPipeline::SelectColorLUT(int index)
{
    if (mColorLUTEntries.empty())
        return;

    const int count = static_cast<int>(mColorLUTEntries.size());
    mColorLUTIndex = (index % count + count) % count;
    mColorLUTEnabled = true;
    ApplyCurrentColorLUT();
}

void GameRenderPipeline::ReloadCurrentColorLUT()
{
    if (mColorLUTIndex < 0 || mColorLUTIndex >= static_cast<int>(mColorLUTEntries.size()))
        return;

    const ColorLUTEntry& entry = mColorLUTEntries[mColorLUTIndex];
    RESOURCEMANAGER.ReloadTexture(entry.ResourceKey, entry.Path);
    ApplyCurrentColorLUT();
}

void GameRenderPipeline::SetColorGrading(const ColorGradingParams& params)
{
    if (mPostProcessPass) mPostProcessPass->SetColorGrading(params);
}

void GameRenderPipeline::SetColorGradingEnabled(bool enabled)
{
    if (mPostProcessPass) mPostProcessPass->SetColorGradingEnabled(enabled);
}


void GameRenderPipeline::SetBlurEnabled(bool on)
{
    mIsBlured = on;
    if (mPostProcessPass) mPostProcessPass->SetBlurEnabled(mIsBlured);
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
    // 패스 함수 자체에 계측 범위를 두어 상위 실행 순서를 단순하게 유지한다.
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Depth));
    GPU_MARKER(L"DepthPrePass");
    // mUseDepthPrePass=false면 forwardOnly=true → FORWARD만 깊이 기록(DEFERRED는 GBuffer가 LESS_EQUAL로 자체 처리)
    mDepthPrePass->Execute(*ctx.deferredBatchs, !mUseDepthPrePass);
}

void GameRenderPipeline::RenderShadow(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Shadow));
    GPU_MARKER(L"ShadowPass");
    static bool sDummyDirty = false;  // ctx 미연결 파이프라인 폴백
    mShadowPass->Execute(
        *ctx.cascadeBatchs,
        const_cast<std::array<bool, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>&>(*ctx.cascadeActive),
        ctx.mapCascadeDirty ? *ctx.mapCascadeDirty : sDummyDirty);
}

void GameRenderPipeline::RenderDeferred(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Deferred));
    // DepthPrePass
    { 
        GPU_MARKER(L"GBuffer"); 
        mGBufferPass->Execute(*ctx.deferredBatchs); 
    }

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

    // HBAO+
    { 
        GPU_MARKER(L"HBAO"); 
        mHBAOPass->Execute(*ctx.deferredBatchs); 
    }

	// Deferred Lighting
    { 
        GPU_MARKER(L"DeferredLighting"); 
        mLightPass->Execute(*ctx.lightBatchs); 
    }

	// Deferred FinalComposite
    {
        GPU_MARKER(L"FinalComposite");

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
    }

    // MotionVector_PS
    { 
        GPU_MARKER(L"MotionVector"); 
        mMotionVectorPass->Execute(*ctx.deferredBatchs); 
    }

    // Depth 상태 복구: DEPTH_READ | PIXEL_SHADER_RESOURCE -> DEPTH_WRITE
    {
        auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
            deferredDepthResource,
            D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
    }
}

void GameRenderPipeline::RenderForward(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Forward));
    GPU_MARKER(L"ForwardPlus");
    mForwardPass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderOutline(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Outline));
    GPU_MARKER(L"Outline");
    mOutlinePass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderDecal(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Decal));
    GPU_MARKER(L"Decal");
    mDecalPass->Execute(ctx.camera);
}

void GameRenderPipeline::RenderEffect(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Effect));
    if (!ctx.camera) return;
    GPU_MARKER(L"Effects");

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
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::Post));
    GPU_MARKER(L"PostProcess");
    mPostProcessPass->Execute(*ctx.deferredBatchs);
}

void GameRenderPipeline::RenderWorldUI(const RenderContext& ctx)
{
    EngineLog::ScopedCpuProfile profile(
        EngineLog::Domain::RenderPassPerformance,
        sPipelineCpuProfile.Get(PipelineProfilePass::WorldUI));
    GPU_MARKER(L"WorldUI");
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
        if (ImGui::Checkbox("Fog", &fogOn))          SetFogEnabled(fogOn);

        bool godRayOn = mGodRayPass ? mGodRayPass->IsEnabled() : false;
        if (ImGui::Checkbox("GodRay (VLS)", &godRayOn))       SetGodRayEnabled(godRayOn);

        bool bloomOn = mEmissiveBloomPass ? mEmissiveBloomPass->IsEnabled() : false;
        if (ImGui::Checkbox("EmissiveBloom", &bloomOn))        SetEmissiveBloomEnabled(bloomOn);

        bool outlineOn = mOutlinePass ? mOutlinePass->IsEnabled() : false;
        if (ImGui::Checkbox("Outline", &outlineOn))      SetOutlineEnabled(outlineOn);

        bool hbaoOn = mHBAOPass ? mHBAOPass->IsEnabled() : false;
        if (ImGui::Checkbox("HBAO+", &hbaoOn))         SetHBAOEnabled(hbaoOn);

        bool fxaaOn = mFXAAPass ? mFXAAPass->IsEnabled() : false;
        if (ImGui::Checkbox("FXAA", &fxaaOn))         SetFXAAEnabled(fxaaOn);

        // 임시 A/B: DepthPrePass on/off (off=GBuffer 자체 깊이, early-Z 없음). GpuProfiler로 active 비교용
        ImGui::Checkbox("DepthPrePass full (uncheck=FORWARD-only)", &mUseDepthPrePass);

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
        float edgeThr = mFXAAPass->GetEdgeThreshold();
        float edgeThrMin = mFXAAPass->GetEdgeThresholdMin();
        float subpix = mFXAAPass->GetSubpixQuality();

        bool changed = false;
        changed |= ImGui::SliderFloat("EdgeThreshold##FXAA", &edgeThr, 0.063f, 0.333f);
        changed |= ImGui::SliderFloat("EdgeThresholdMin##FXAA", &edgeThrMin, 0.031f, 0.083f);
        changed |= ImGui::SliderFloat("SubpixQuality##FXAA", &subpix, 0.0f, 1.0f);

        if (changed)
            mFXAAPass->SetParams(edgeThr, edgeThrMin, subpix);
    }

    // ── ColorGrading ─────────────────────────────────────────────────────────
    if (mPostProcessPass && ImGui::CollapsingHeader("ColorGrading"))
    {
        ColorGradingParams p = mPostProcessPass->GetColorGrading();
        bool colorGradingEnabled = mPostProcessPass->IsColorGradingEnabled();
        bool changed = false;

        if (ImGui::Checkbox("Custom ColorGrading##CG", &colorGradingEnabled))
            SetColorGradingEnabled(colorGradingEnabled);

        changed |= ImGui::SliderFloat("Saturation##CG", &p.Saturation, 0.0f, 3.0f);
        changed |= ImGui::SliderFloat("Contrast##CG", &p.Contrast, 0.0f, 3.0f);
        changed |= ImGui::SliderFloat("Brightness##CG", &p.Brightness, -1.0f, 1.0f);
        changed |= ImGui::SliderFloat("Exposure##CG", &p.Exposure, 0.1f, 5.0f);

        ImGui::Separator();
        float shadow[3] = { p.ShadowTint.x, p.ShadowTint.y, p.ShadowTint.z };
        if (ImGui::ColorEdit3("ShadowTint##CG", shadow))
        {
            p.ShadowTint = Vec3(shadow[0], shadow[1], shadow[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("ShadowStrength##CG", &p.ShadowStrength, 0.0f, 1.0f);

        float mid[3] = { p.MidtoneTint.x, p.MidtoneTint.y, p.MidtoneTint.z };
        if (ImGui::ColorEdit3("MidtoneTint##CG", mid))
        {
            p.MidtoneTint = Vec3(mid[0], mid[1], mid[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("MidtoneStrength##CG", &p.MidtoneStrength, 0.0f, 1.0f);

        float hi[3] = { p.HighlightTint.x, p.HighlightTint.y, p.HighlightTint.z };
        if (ImGui::ColorEdit3("HighlightTint##CG", hi))
        {
            p.HighlightTint = Vec3(hi[0], hi[1], hi[2]);
            changed = true;
        }
        changed |= ImGui::SliderFloat("HighlightStrength##CG", &p.HighlightStrength, 0.0f, 1.0f);

        if (changed)
            mPostProcessPass->SetColorGrading(p);

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Color LUT", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (mColorLUTEntries.empty())
            {
                ImGui::TextUnformatted("No LUT files found in Resources/Texture/LUTs");
            }
            else
            {
                if (ImGui::Checkbox("Enabled##LUT", &mColorLUTEnabled))
                    SetColorLUTEnabled(mColorLUTEnabled);

                if (ImGui::Button("Previous##LUT"))
                    SelectColorLUT(mColorLUTIndex - 1);
                ImGui::SameLine();
                if (ImGui::Button("Next##LUT"))
                    SelectColorLUT(mColorLUTIndex + 1);
                ImGui::SameLine();
                if (ImGui::Button("Reload current##LUT"))
                    ReloadCurrentColorLUT();
                ImGui::SameLine();
                if (ImGui::Button("Rescan files##LUT"))
                    DiscoverColorLUTs();

                if (mColorLUTEntries.empty())
                {
                    ImGui::TextUnformatted("No LUT files found in Resources/Texture/LUTs");
                }
                else
                {
                    const string preview = ws2s(mColorLUTEntries[mColorLUTIndex].DisplayName);
                    if (ImGui::BeginCombo("Current LUT##LUT", preview.c_str()))
                    {
                        
                        static ImGuiTextFilter lutFilter;
                        lutFilter.Draw("Filter##LUT", 260.0f);
                        for (int index = 0; index < static_cast<int>(mColorLUTEntries.size()); ++index)
                        {
                            const bool selected = index == mColorLUTIndex;
                            const string name = ws2s(mColorLUTEntries[index].DisplayName);
                            if (!lutFilter.PassFilter(name.c_str()))
                                continue;
                            if (ImGui::Selectable(name.c_str(), selected))
                                SelectColorLUT(index);
                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::SliderFloat("Strength##LUT", &mColorLUTStrength, 0.0f, 1.0f))
                        ApplyCurrentColorLUT();
                    ImGui::Text("LUT %d of %d", mColorLUTIndex + 1, static_cast<int>(mColorLUTEntries.size()));
                }
            }
        }
    }

    ImGui::End();
#endif
}
