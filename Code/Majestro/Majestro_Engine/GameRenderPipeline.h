#pragma once
#include "RenderPass.h"

class DepthPrePass;
class ShadowPass;
class GBufferPass;
class LightsPass;
class ForwardPass;
class OutlinePass;
class DecalPass;
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

    void PreCompute(const RenderContext& ctx) override;
    void ExecuteIndependentGraphics(const RenderContext& ctx) override;
    void ExecuteDependentGraphics(const RenderContext& ctx) override;
    void Execute(const RenderContext& ctx)    override;

    

    // 이벤트 기반 PostProcess on/off
    void SetMotionBlurEnabled(bool on);
    void SetFogEnabled(bool on);
    void SetOutlineEnabled(bool on);
    void SetGodRayEnabled(bool on);
    void SetEmissiveBloomEnabled(bool on);
    void SetBlurEnabled(bool on);
    void SetFXAAEnabled(bool on);
    void SetFXAAParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality);


    void SetColorLUTEnabled(bool enabled);
    void SetColorGradingEnabled(bool enabled);
	void SetColorLUT(const std::wstring& name, int size, float strength = 1.0f);    // LUT 이름, 3D LUT 크기, 강도(0~1)
	void SetColorGrading(const ColorGradingParams& params);         // ColorGradingParams 구조체로 색보정 파라미터 설정
    
   
	void DiscoverColorLUTs();       // LUT 리소스 폴더를 스캔하여 목록 갱신
	void ApplyCurrentColorLUT();    // 현재 선택된 LUT를 PostProcessPass에 적용
	void SelectColorLUT(int index); // LUT 목록에서 인덱스 선택, ApplyCurrentColorLUT() 호출
	void ReloadCurrentColorLUT();   // 현재 선택된 LUT를 다시 로드, ApplyCurrentColorLUT() 호출

    void SetHealthVignetteNoiseTexture(const std::wstring& textureName);
    void SetWorldUIFeature(std::vector<shared_ptr<UIFeature>>* features);

    GodRayPass*          GetGodRayPass()    const { return mGodRayPass.get(); }
    DualKawaseBlurPass*  GetEmissiveBloom() const { return mEmissiveBloomPass.get(); }
    HBAOPass*            GetHBAOPass()      const { return mHBAOPass.get(); }
    FXAAPass*            GetFXAAPass()      const { return mFXAAPass.get(); }

    void SetHBAOEnabled(bool on);

    // 현재 Pass on/off 조회
    bool IsFogEnabled()     const;
    bool IsGodRayEnabled()  const;
    bool IsBloomEnabled()   const;
    bool IsOutlineEnabled() const;
    bool IsHBAOEnabled()    const;
    bool IsFXAAEnabled()    const;
    bool IsColorLUTEnabled() const;
    bool IsColorGradingEnabled() const;

    // RenderManager::GetGraphicsSettings() 값을 일괄 반영.
    void ApplyGraphicsSettings();

    // 동적 HDR PostProcess 추가/제거
    void AddHDREffect(shared_ptr<class RenderPass> pass);
    void RemoveHDREffect(shared_ptr<class RenderPass> pass);

private:

    struct ColorLUTEntry
    {
        wstring DisplayName;
        wstring ResourceKey;
        wstring Path;
    };

    shared_ptr<DepthPrePass>     mDepthPrePass;
    shared_ptr<ShadowPass>       mShadowPass;
    shared_ptr<GBufferPass>      mGBufferPass;
    shared_ptr<LightsPass>       mLightPass;
    shared_ptr<ForwardPass>      mForwardPass;
    shared_ptr<OutlinePass>      mOutlinePass;
    shared_ptr<DecalPass>        mDecalPass;
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
    bool   mIsBlured = false;
    bool   mUseDepthPrePass = true;   // 임시 A/B 측정용: off면 DepthPrePass 생략하고 GBuffer가 LESS+Write로 자체 깊이 생성

    bool mHasPreviousHealthRatio = false;
    float mPreviousHealthRatio = 1.0f;
    float mHealthVignetteLowStrength = 0.0f;
    float mHealthVignetteHealTimer = 0.0f;
    float mHealthVignetteLowThreshold = 0.55f;
    float mHealthVignetteHealDuration = 0.75f;
    Vec4 mBuffIconStrengths = Vec4::Zero;
    std::vector<ColorLUTEntry> mColorLUTEntries;
    int mColorLUTIndex = -1;
    float mColorLUTStrength = 1.0f;
    bool mColorLUTEnabled = true;
private:

    void UpdatePassStates();
    void UpdateHealthVignetteState();
    void RenderDepthPrePass(const RenderContext& ctx);
    void RenderShadow(const RenderContext& ctx);
    void RenderDeferred(const RenderContext& ctx);
    void RenderForward(const RenderContext& ctx);
    void RenderOutline(const RenderContext& ctx);
    void RenderDecal(const RenderContext& ctx);
    void RenderEffect(const RenderContext& ctx);
    void RenderPost(const RenderContext& ctx);
    void RenderWorldUI(const RenderContext& ctx);
};
