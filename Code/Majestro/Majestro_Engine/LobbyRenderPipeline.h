#pragma once
#include "RenderPass.h"

class DepthPrePass;
class ForwardPass;
class EffectPass;
class PostProcessPass;
class FogPass;
class GBufferPass;
class LightsPass;
class ShadowPass;
class OutlinePass;
class DualKawaseBlurPass;
class LobbyBackgroundPass;

// 로비 / 메인메뉴 / 로딩 / 결과 씬용 간소 파이프라인
// ForwardPass / EffectPass / PostProcess(ToneMap + FinalComposite)
class LobbyRenderPipeline : public IRenderPipeline
{
public:
    LobbyRenderPipeline()  = default;
    ~LobbyRenderPipeline() = default;

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
    void RenderDeferred(const RenderContext& ctx);
private:
    shared_ptr<DepthPrePass>        mDepthPrePass;
    shared_ptr<ShadowPass>          mShadowPass;
    shared_ptr<ForwardPass>         mForwardPass;
    shared_ptr<EffectPass>          mEffectPass;
    shared_ptr<PostProcessPass>     mPostProcessPass;
    shared_ptr<FogPass>             mFogPass;
    shared_ptr<GBufferPass>         mGBufferPass;
    shared_ptr<LightsPass>          mLightPass;
    shared_ptr<OutlinePass>         mOutlinePass;
    shared_ptr<DualKawaseBlurPass>  mEmissiveBloomPass;
    shared_ptr<LobbyBackgroundPass> mLobbyBackgroundPass;


    World* mWorld = nullptr;
};
