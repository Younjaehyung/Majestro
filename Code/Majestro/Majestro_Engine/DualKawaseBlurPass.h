#pragma once
#include "RenderPass.h"
#include "Texture.h"

// ──────────────────────────────────────────────────────────────
// Dual Kawase Blur 기반 이미시브 블룸 패스
//
// 파이프라인 역할:
//   mBefore HDR  -> [Bright Extract] -> [Downsample ×N] -> [Upsample ×N]
//   -> [Additive Composite] -> mAfter HDR
//
// 사용 예:
//   auto bloomPass = make_shared<DualKawaseBlurPass>();
//   bloomPass->Initialize(4);
//   bloomPass->SetThreshold(1.0f);
//   bloomPass->SetIntensity(0.8f);
//   mPostProcessPass->AddHDRPass(bloomPass);
//
// PASS_CUSTOM_DATA 레이아웃:
//   POST_KAWASE_EXTRACT_PASS (=12)
//     PreviousStep  : mBefore RT의 GBuffer 인덱스
//     ExtValue[0]   : (threshold, knee, 0, 0)
//
//   POST_KAWASE_DOWN_PASS (=13)
//     ExtValue[step] : (texelW, texelH, float(srcImageIdx), 0)  (step = 0..N-1)
//
//   POST_KAWASE_UP_PASS (=14)
//     ExtValue[step] : (texelW, texelH, float(srcImageIdx), 0)  (step = 0..N-1)
//
//   POST_KAWASE_COMPOSITE_PASS (=15)
//     PreviousStep  : mBefore RT의 GBuffer 인덱스 (씬 컬러)
//     ExtTex[0]     : 최종 업샘플 결과의 TextureMaps 이미지 인덱스
//     ExtValue[0]   : (intensity, 0, 0, 0)
//
// GlobalParams 사용:
//   etc     (offset 1) : 현재 step 인덱스 (0~N-1), 다운/업샘플 단계마다 갱신
// ──────────────────────────────────────────────────────────────

// 중간 렌더 타깃 정보 (Pass 내부 전용)
struct KawaseRT
{
    shared_ptr<Texture>         tex;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    uint32                      width  = 0;
    uint32                      height = 0;
};

class DualKawaseBlurPass : public RenderPass
{
public:
    DualKawaseBlurPass()  = default;
    ~DualKawaseBlurPass() = default;

    // iterations: 다운/업샘플 반복 횟수 (1~4, 기본 4)
    void Initialize(int iterations = 4);

    void SetData(
        std::array<PassCustomData,
        static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before,
        RENDER_TARGET_GROUP_TYPE after) override;

    void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;


    // 이미시브 강도 스케일 (GBuffer 이미시브 값에 곱하는 배율, 기본 1.0)
    void  SetThreshold(float t) { mThreshold = t; }
    float GetThreshold() const  { return mThreshold; }

    // 최종 합성 강도 (기본 0.8)
    void  SetIntensity(float i) { mIntensity = i; }
    float GetIntensity() const  { return mIntensity; }

private:

    void RunExtract();
    void RunDownsample();
    void RunUpsample();
    void RunComposite();


    KawaseRT CreateKawaseRT(const wstring& name, uint32 w, uint32 h);

    void SetViewportAndScissor(uint32 w, uint32 h);
    void RestoreViewportAndScissor();

    void TransitionTo(ID3D12Resource* res,
                      D3D12_RESOURCE_STATES from,
                      D3D12_RESOURCE_STATES to);

    void ClearAndBindRTV(const KawaseRT& rt);


    int   mIterations = 4;
    float mThreshold  = 1.0f;
    float mIntensity  = 0.8f;


    KawaseRT         mExtractRT;
    vector<KawaseRT> mDownChain; // [0]=W/2  [1]=W/4  [2]=W/8  [3]=W/16
    vector<KawaseRT> mUpChain;   // [0]=W/8  [1]=W/4  [2]=W/2  [3]=W   (N개)


    static constexpr uint32 EXTRACT_IDX   =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_KAWASE_EXTRACT_PASS);
    static constexpr uint32 DOWN_IDX      =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_KAWASE_DOWN_PASS);
    static constexpr uint32 UP_IDX        =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_KAWASE_UP_PASS);
    static constexpr uint32 COMPOSITE_IDX =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_KAWASE_COMPOSITE_PASS);
};
