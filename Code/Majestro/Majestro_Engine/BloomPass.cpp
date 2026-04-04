#include "pch.h"
#include "BloomPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

// ─────────────────────────────────────────────────────────────────────────────
//  내부 헬퍼: 각 MIP 레벨의 텍셀 크기 계산
//   mip=0 → 1/2 해상도, mip=1 → 1/4 해상도 ...
// ─────────────────────────────────────────────────────────────────────────────
static std::pair<float, float> CalcTexelSize(uint32 screenW, uint32 screenH, int mip)
{
    float scale = static_cast<float>(1 << (mip + 1)); // 2^(mip+1)
    return { scale / static_cast<float>(screenW), scale / static_cast<float>(screenH) };
}

void BloomPass::Initialize()
{
}

void BloomPass::SetData(
    std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter  = after;

    const auto& win = RENDERMANAGER.GetWindow();

    // ── [1] Bright Pass: HDR SceneColor → BLOOM_MIP0 ───────────────────
    {
        auto& bright         = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_BRIGHT_PASS)];
        bright.PreviousStep  = static_cast<int32>(ToGBufferIndex(mBefore)); // HDR SceneColor
        // ExtValue[0]: (threshold, knee, prefilterStrength, unused)
        bright.ExtValue[0]   = Vec4(mThreshold, mKnee, mPrefilterStrength, 0.f);
    }

    // ── [2] Downsample: 각 단계는 Execute 시 root constant로 srcMip 전달 ──
    //    텍셀 크기는 Execute에서 SetData 재호출 없이 root constant로 전달.
    //    SetData에서는 upsample strength만 미리 세팅해 둔다.
    {
        auto& ds         = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_DS_PASS)];
        // ExtTex[0], ExtValue[0] = Execute에서 매 단계 직접 업데이트
        (void)ds;
    }

    // ── [3] Upsample: 각 단계는 Execute 시 srcMip + 텍셀 크기 전달 ───────
    {
        auto& us         = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_US_PASS)];
        // ExtValue[0].z = upsampleStrength (런타임 변경용으로 Execute에서도 설정)
        us.ExtValue[0].z = mUpsampleStrength;
        (void)us;
    }

    // ── [4] Composite: HDR + BLOOM_MIP0 → mAfter ──────────────────────
    {
        auto& comp         = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_COMP_PASS)];
        comp.PreviousStep  = static_cast<int32>(ToGBufferIndex(mBefore)); // HDR SceneColor
        // ExtValue[0]: (bloomIntensity, unused, unused, unused)
        comp.ExtValue[0]   = Vec4(mBloomIntensity, 0.f, 0.f, 0.f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PassCustomTable 행을 실행 시점에 직접 업데이트하는 헬퍼
//  (SetData는 Initialize 시 한 번만 호출되므로 동적 값은 Execute에서 갱신)
// ─────────────────────────────────────────────────────────────────────────────
static void UpdatePassCustomRow(
    PASS_CUSTOM_INDEX idx,
    int32  extTex0,
    float  srcTw, float srcTh,
    float  extraParam,   // DS: useKaris  /  US: upsampleStrength
    int32  prevStep = -1)
{
    // GroupBuffer의 PassCustomTableInfo structured buffer 직접 수정
    // → RenderPass 공통 SetData 인터페이스 우회
    // 기존 엔진 패턴: dataTable 포인터가 없으므로 PassCustomTable을 직접 수정하는
    // 간단한 방법 대신, root32bit constant로 srcMip을 전달하고
    // ExtValue는 SetData에서 미리 설정한 값을 런타임에 override한다.
    //
    // [주의] 실제 엔진에서 PassCustomTable이 GPU 업로드 전에 CPU에서 수정 가능하다면
    //        아래처럼 gPassCustomTable[idx]에 직접 접근하는 방식을 쓴다.
    //        그렇지 않으면 Shader::Update() 이전에 SetData()를 매 프레임 다시 호출해야 함.
    //
    // 이 엔진에서는 RenderPass::SetData가 프레임 시작 시 호출됨(PostProcessPass::SetData).
    // 여기서는 root32bit constant(GlobalParams.etc, casdcae)로 srcMip과 boolean을 전달하고,
    // 텍셀 크기만 PassCustomTable의 ExtValue에 매 패스 직전 업데이트한다.
    //
    // → RENDERMANAGER의 GroupBuffer를 통해 PassCustomData를 업데이트하거나,
    //   또는 Execute 진입 전 SetData를 다시 호출하도록 PostProcessPass::Execute를 수정한다.
    //
    // 현재 구현: ExtValue를 직접 쓰는 대신 GlobalParams.etc / casdcae로 전달.
    (void)idx; (void)extTex0; (void)srcTw; (void)srcTh; (void)extraParam; (void)prevStep;
}

void BloomPass::Execute(std::vector<DrawBatch>&)
{
    if (!mEnabled) return;

    const auto& win = RENDERMANAGER.GetWindow();

    // MIP 레벨별 텍셀 크기 (다운샘플 소스 기준)
    // mip=0: MIP0 (1/2해상도) 텍셀
    // mip=1: MIP1 (1/4해상도) 텍셀 ...
    auto [tw0, th0] = CalcTexelSize(win.Width, win.Height, 0);
    auto [tw1, th1] = CalcTexelSize(win.Width, win.Height, 1);
    auto [tw2, th2] = CalcTexelSize(win.Width, win.Height, 2);
    auto [tw3, th3] = CalcTexelSize(win.Width, win.Height, 3);

    // ── [1] Bright Pass: HDR SceneColor → BLOOM_MIP0 ──────────────────
    {
        auto& mip0 = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::BLOOM_MIP0));
        mip0.ClearRenderTargetView();
        mip0.OMSetRenderTargets(); // 뷰포트 자동으로 W/2 × H/2 로 설정됨

        uint32 passIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_BRIGHT_PASS);
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3); // PassCustomIndex
        RESOURCEMANAGER.Get<Shader>(L"BloomBright")->Update();
        RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
        mip0.WaitTargetToResource();
    }

    // ── [2] Downsample 체인 ──────────────────────────────────────────────
    //  소스: BloomMips[srcMip]  →  목적지: BloomMips[srcMip+1]
    //  GlobalParams.etc = srcMip (0~2)
    //  GlobalParams.casdcae = useKaris (1 = 첫 패스만 적용)
    {
        uint32 passIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_DS_PASS);

        // 다운샘플 단계별 설정: (srcMipIdx, srcTw, srcTh, useKaris, dstGroupType)
        struct DsStep { uint32 srcMip; float tw, th; uint32 useKaris; RENDER_TARGET_GROUP_TYPE dst; };
        const DsStep steps[3] = {
            // MIP0 → MIP1
            { 0, tw0, th0, 1, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP1 },
            // MIP1 → MIP2
            { 1, tw1, th1, 0, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP2 },
            // MIP2 → MIP3
            { 2, tw2, th2, 0, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP3 },
        };

        for (auto& step : steps)
        {
            // PassCustomIndex
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);
            // GlobalParams.etc = srcMip 인덱스 (HLSL에서 BloomMips[etc] 로 읽음)
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &step.srcMip, 1);
            // GlobalParams.casdcae = useKaris
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &step.useKaris, 2);

            // 텍셀 크기를 PassCustomTable[POST_BLOOM_DS_PASS].ExtValue[0]에 업데이트
            // 현재 엔진 구조상 PassCustomData는 프레임당 SetData에서 한 번 업로드됨.
            // 다운샘플 단계별로 다른 텍셀 크기가 필요하므로,
            // HLSL에서 GlobalParams.etc (srcMip)를 이용해 직접 계산하거나
            // 아래처럼 PassCustomTable의 ExtValue를 매 패스 직전 갱신해야 한다.
            //
            // [간단한 대안] HLSL에서 srcMip으로 텍셀 크기 직접 계산:
            //   float2 srcRes = PassParams.ScreenSize / pow(2, etc + 1);
            //   float2 ts = 1.0 / srcRes;
            // → bloom_downsample_PS.hlsl의 ExtValue 대신 이 방법 권장.
            // (아래 코드는 ExtValue 방식 주석 처리, HLSL에서 자동 계산 방식으로 대체)

            auto& dst = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(step.dst));
            dst.ClearRenderTargetView();
            dst.OMSetRenderTargets(); // 뷰포트 자동으로 목적 해상도에 맞게 설정
            RESOURCEMANAGER.Get<Shader>(L"BloomDownsample")->Update();
            RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
            dst.WaitTargetToResource();
        }
    }

    // ── [3] Upsample 누적 (additive blend: ONE_TO_ONE_BLEND) ─────────────
    //  MIP3(leaf) → MIP2 → MIP1 → MIP0
    //  목적지 RT를 Clear하지 않는다 — 기존 downsampled 값 위에 additive.
    //  BloomUpsample 쉐이더는 반드시 ONE_TO_ONE_BLEND로 등록되어야 한다.
    {
        uint32 passIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_US_PASS);

        // 업샘플 단계별 설정: (srcMipIdx, srcTw, srcTh, dstGroupType)
        struct UsStep { uint32 srcMip; float tw, th; RENDER_TARGET_GROUP_TYPE dst; };
        const UsStep steps[3] = {
            // MIP3 → MIP2 (souce=MIP3 텍셀)
            { 3, tw3, th3, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP2 },
            // MIP2 → MIP1
            { 2, tw2, th2, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP1 },
            // MIP1 → MIP0
            { 1, tw1, th1, RENDER_TARGET_GROUP_TYPE::BLOOM_MIP0 },
        };

        for (auto& step : steps)
        {
            // PassCustomIndex
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);
            // GlobalParams.etc = srcMip
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &step.srcMip, 1);

            auto& dst = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(step.dst));
            // ★ Clear 하지 않는다 — additive blend가 기존 downsampled 값을 보존
            dst.WaitResourceToTarget(); // SRV → RTV 전환 (다운샘플에서 WaitTargetToResource 후)
            dst.OMSetRenderTargets();   // 뷰포트 자동으로 목적 해상도에 맞게 설정
            RESOURCEMANAGER.Get<Shader>(L"BloomUpsample")->Update();
            RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
            dst.WaitTargetToResource();
        }
    }

    // ── [4] Composite: HDR + BLOOM_MIP0 → mAfter ──────────────────────
    //  이 결과가 ToneMapPass의 입력이 됨.
    {
        auto& afterGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter));
        afterGroup.ClearRenderTargetView();
        afterGroup.OMSetRenderTargets(); // 풀해상도로 복원

        uint32 compIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_BLOOM_COMP_PASS);
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &compIdx, 3);
        RESOURCEMANAGER.Get<Shader>(L"BloomComposite")->Update();
        RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
        afterGroup.WaitTargetToResource();
    }
}
