#pragma once
#include "RenderPass.h"

class DepthPrePass;
class ShadowPass;
class GBufferPass;
class LightsPass;
class ForwardPass;
class OutlinePass;
class EffectPass;
class ParticlePass;
class TrailRenderPass;
class PostProcessPass;
class MotionVectorPass;
class FogPass;
class MotionBlurPass;
class LuminancePass;
class GodRayPass;
class DualKawaseBlurPass;
class HBAOPass;
class FXAAPass;
class HealthVignettePass;
class WorldUIPass;
class UIFeature;


// DepthPrePass / Shadow / GBuffer / Deferred Lighting /
// Forward+ / Outline / Effect(Effekseer) / PostProcess
class GameRenderPipeline : public IRenderPipeline
{
public:
    GameRenderPipeline()  = default;
    ~GameRenderPipeline() = default;

    void Initialize(World* world)   override;
    void OnResize(uint32 w, uint32 h) override;
    void DrawImGui()                override;

    void SetupPassTable(
        std::array<PassCustomData,
        static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& table) override;

    void PreCompute(const RenderContext& ctx) override; // ForwardPlus Cull Compute
    void Execute(const RenderContext& ctx)    override;

    
    // PauseMenu 진입 시 게임 월드 렌더 생략, UI 오버레이만 실행
    void SetPaused(bool paused) { mIsPaused = paused; }
    bool IsPaused() const       { return mIsPaused; }

    // 이벤트 기반 PostProcess on/off
    void SetMotionBlurEnabled(bool on);
    void SetFogEnabled(bool on);
    void SetOutlineEnabled(bool on);
    void SetGodRayEnabled(bool on);
    void SetEmissiveBloomEnabled(bool on);
    void SetFXAAEnabled(bool on);
    void SetFXAAParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality);
    void SetHealthVignetteNoiseTexture(const std::wstring& textureName);
	void SetWorldUIFeature(std::vector<shared_ptr<UIFeature>>* features);

    GodRayPass*          GetGodRayPass()    const { return mGodRayPass.get(); }
    DualKawaseBlurPass*  GetEmissiveBloom() const { return mEmissiveBloomPass.get(); }
    HBAOPass*            GetHBAOPass()      const { return mHBAOPass.get(); }
    FXAAPass*            GetFXAAPass()      const { return mFXAAPass.get(); }

    void SetHBAOEnabled(bool on);

    // 동적 HDR PostProcess 추가/제거
    void AddHDREffect(shared_ptr<class RenderPass> pass);
    void RemoveHDREffect(shared_ptr<class RenderPass> pass);

private:

    shared_ptr<DepthPrePass>     mDepthPrePass;
    shared_ptr<ShadowPass>       mShadowPass;
    shared_ptr<GBufferPass>      mGBufferPass;
    shared_ptr<LightsPass>       mLightPass;
    shared_ptr<ForwardPass>      mForwardPass;
    shared_ptr<OutlinePass>      mOutlinePass;
    shared_ptr<EffectPass>       mEffectPass;
    shared_ptr<TrailRenderPass>  mTrailRenderPass;
    shared_ptr<ParticlePass>     mParticlePass;
    shared_ptr<PostProcessPass>  mPostProcessPass;
    shared_ptr<MotionVectorPass> mMotionVectorPass;
    shared_ptr<FogPass>          mFogPass;
    shared_ptr<MotionBlurPass>   mMotionBlurPass;
    shared_ptr<LuminancePass>    mLuminancePass;
    shared_ptr<GodRayPass>          mGodRayPass;
    shared_ptr<DualKawaseBlurPass>  mEmissiveBloomPass;
    shared_ptr<HBAOPass>            mHBAOPass;
    shared_ptr<HealthVignettePass>  mHealthVignettePass;
    shared_ptr<FXAAPass>            mFXAAPass;
    shared_ptr<WorldUIPass>         mWorldUIPass;

    World* mWorld    = nullptr;
    bool   mIsPaused = false;

    bool mHasPreviousHealthRatio = false;
    float mPreviousHealthRatio = 1.0f;
    float mHealthVignetteLowStrength = 0.0f;
    float mHealthVignetteHealTimer = 0.0f;
    float mHealthVignetteLowThreshold = 0.55f;
    float mHealthVignetteHealDuration = 0.75f;
private:

    void UpdatePassStates();
    void UpdateHealthVignetteState();
    void RenderDepthPrePass(const RenderContext& ctx);
    void RenderShadow(const RenderContext& ctx);
    void RenderDeferred(const RenderContext& ctx);
    void RenderForward(const RenderContext& ctx);
    void RenderOutline(const RenderContext& ctx);
    void RenderEffect(const RenderContext& ctx);
    void RenderPost(const RenderContext& ctx);
    void RenderWorldUI(const RenderContext& ctx);
};
