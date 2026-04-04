#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

// ─────────────────────────────────────────────────────────────────────────────
// BloomPass — HDR 멀티스케일 피라미드 Bloom
//
//  처리 순서 (모두 ToneMap 이전 HDR 공간에서 수행):
//
//   [1] Bright Pass   : HDR SceneColor → BLOOM_MIP0 (1/2 해상도)
//                        soft-knee threshold로 발광 영역 추출
//
//   [2] Downsample    : MIP0→MIP1→MIP2→MIP3  (13-tap Karis average)
//
//   [3] Upsample      : MIP3→MIP2→MIP1→MIP0  (tent filter, additive blend)
//                        각 단계에서 downsampled 기여분 위에 누적
//
//   [4] Composite     : HDR SceneColor + BLOOM_MIP0 → output HDR RT
//                        이 결과를 ToneMapPass가 읽음
//
//  에미시브가 HDR SceneColor에 이미 합산되어 있으므로
//  별도 이미시브 GBuffer를 읽을 필요 없음.
// ─────────────────────────────────────────────────────────────────────────────
class BloomPass : public RenderPass
{
public:
    BloomPass() = default;
    virtual ~BloomPass() = default;

    virtual void Initialize() override;
    virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
    virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;

    // ── 파라미터 ────────────────────────────────────────────────────────
    // Bright Pass
    void SetThreshold(float v)        { mThreshold        = v; }  // HDR 단위 (권장 1.0~1.5)
    void SetKnee(float v)             { mKnee             = v; }  // soft-knee 폭 (권장 threshold*0.5)
    void SetPrefilterStrength(float v){ mPrefilterStrength = v; }  // 추출 강도 배율 (기본 1.0)

    // Upsample / Composite
    void SetUpsampleStrength(float v) { mUpsampleStrength  = v; }  // 업샘플 기여 배율 (권장 0.85)
    void SetBloomIntensity(float v)   { mBloomIntensity    = v; }  // 최종 합산 강도 (권장 1.0)

    // 접근자
    float GetThreshold()        const { return mThreshold; }
    float GetBloomIntensity()   const { return mBloomIntensity; }

private:
    // Bright Pass
    float mThreshold         = 1.0f;   // HDR 기준: 1.0 이상이 bloom 기여
    float mKnee              = 0.5f;   // soft-knee 폭 (threshold * 0.5 권장)
    float mPrefilterStrength = 1.0f;

    // Upsample / Composite
    float mUpsampleStrength  = 0.85f;  // 업샘플 단계별 기여 배율
    float mBloomIntensity    = 1.0f;   // 최종 HDR + bloom 합산 배율

    // Downsample/Upsample 설정은 Execute 시 mipIdx에서 동적 계산
};
