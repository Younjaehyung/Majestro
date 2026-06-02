#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

// ──────────────────────────────────────────────────────────────
// FXAA (Fast Approximate Anti-Aliasing) LDR 포스트프로세스 패스
//
// 파이프라인 역할:
//   POST_LDR_A (ToneMap 결과) → [FXAA] → POST_LDR_B
//   → FinalCompositePass → SwapChain
//
// PostProcessPass의 LDR 패스 체인에 등록:
//   mPostProcessPass->AddLDRPass(mFXAAPass);
//
// PASS_CUSTOM_DATA 레이아웃 (POST_FXAA_PASS = 20):
//   PreviousStep  : 입력 LDR RT의 Gbuffer[] 인덱스
//                   (PostProcessPass ping-pong이 자동 설정)
//   ExtValue[0].x : edgeThreshold    — 엣지 감지 최소 루마 대비
//                   (기본 0.125: 대비가 이 값 미만이면 FXAA 생략)
//   ExtValue[0].y : edgeThresholdMin — 어두운 영역 컷오프
//                   (기본 0.0625: 매우 어두운 픽셀 불필요한 처리 방지)
//   ExtValue[0].z : subpixQuality    — 서브픽셀 블렌드 강도
//                   (기본 0.75: 0=꺼짐, 1=최대, 과하면 선이 흐려짐)
//
// ──────────────────────────────────────────────────────────────
class FXAAPass : public RenderPass
{
public:
    FXAAPass()  = default;
    ~FXAAPass() = default;

    void Initialize() override;

    void SetData(
        std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before,
        RENDER_TARGET_GROUP_TYPE after) override;

    void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;

    // ── 파라미터 조정 API ──────────────────────────────────────
    // edgeThreshold    : 0.063(품질 최고) ~ 0.333(성능 최고), 권장 0.125
    // edgeThresholdMin : 0.031(품질 최고) ~ 0.0833(성능 최고), 권장 0.0625
    // subpixQuality    : 0.0(꺼짐) ~ 1.0(최대 블렌드), 권장 0.75
    void SetParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality);

    float GetEdgeThreshold()    const { return mEdgeThreshold;    }
    float GetEdgeThresholdMin() const { return mEdgeThresholdMin; }
    float GetSubpixQuality()    const { return mSubpixQuality;    }

private:
    static constexpr uint32 FXAA_IDX =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FXAA_PASS);

    float mEdgeThreshold    = 0.125f;
    float mEdgeThresholdMin = 0.0625f;
    float mSubpixQuality    = 0.75f;
};
