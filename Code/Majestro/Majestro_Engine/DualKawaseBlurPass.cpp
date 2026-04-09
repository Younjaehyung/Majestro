#include "pch.h"
#include "DualKawaseBlurPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"


void DualKawaseBlurPass::Initialize(int iterations)
{
    mIterations = std::clamp(iterations, 1, 4);

    uint32 sw = static_cast<uint32>(RENDERMANAGER.GetViewPort().Width);
    uint32 sh = static_cast<uint32>(RENDERMANAGER.GetViewPort().Height);

    // 이미시브 추출 RT (원본 해상도)
    mExtractRT = CreateKawaseRT(L"KawaseExtract", sw, sh);

    // 다운샘플 체인: [i] = W/2^(i+1)
    mDownChain.clear();
    for (int i = 0; i < mIterations; ++i)
    {
        uint32 w = max(sw >> (i + 1), 1u);
        uint32 h = max(sh >> (i + 1), 1u);
        mDownChain.push_back(
            CreateKawaseRT(L"KawaseDown_" + to_wstring(i), w, h));
    }

    // 업샘플 체인: [i] = W/2^(N-1-i)  ->  up[0]=W/8, up[N-1]=W 순서
    mUpChain.clear();
    for (int i = 0; i < mIterations; ++i)
    {
        uint32 w = max(sw >> (mIterations - 1 - i), 1u);
        uint32 h = max(sh >> (mIterations - 1 - i), 1u);
        mUpChain.push_back(
            CreateKawaseRT(L"KawaseUp_" + to_wstring(i), w, h));
    }
}

void DualKawaseBlurPass::SetData(
    std::array<PassCustomData,
    static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before,
    RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter  = after;

    float sw = RENDERMANAGER.GetViewPort().Width;
    float sh = RENDERMANAGER.GetViewPort().Height;

    // EXTRACT
    // PreviousStep 미사용 (Gbuffer[4] 직접 읽기)
    // ExtValue[0].x = 이미시브 강도 스케일 (mThreshold를 스케일로 재활용)
    {
        auto& d = dataTable[EXTRACT_IDX];
        d.ExtValue[0] = Vec4(mThreshold, 0.f, 0.f, 0.f);
    }

    // DOWNSAMPLE
    // ExtValue[step] = (texelW, texelH, float(srcImageIdx), 0)
    // step 0: 소스 = Extract (W×H),   texelSize = 1/W, 1/H
    // step i: 소스 = Down[i-1] (W/2^i × H/2^i)
    {
        auto& d = dataTable[DOWN_IDX];
        for (int i = 0; i < mIterations; ++i)
        {
            float srcW = sw / float(1 << i);
            float srcH = sh / float(1 << i);

            uint32 srcIdx = (i == 0)
                ? mExtractRT.tex->GetImageIndex()
                : mDownChain[i - 1].tex->GetImageIndex();

            d.ExtValue[i] = Vec4(1.f / srcW, 1.f / srcH,
                                 static_cast<float>(srcIdx), 0.f);
        }
    }

    //UPSAMPLE
    // step 0: 소스 = Down[N-1] (W/2^N × H/2^N)
    // step i: 소스 = Up[i-1]   (W/2^(N-i) × H/2^(N-i))
    {
        auto& d = dataTable[UP_IDX];
        for (int i = 0; i < mIterations; ++i)
        {
            int   srcLevel = mIterations - i;           // 소스 레벨 (분모 = 2^srcLevel)
            float srcW     = sw / float(1 << srcLevel);
            float srcH     = sh / float(1 << srcLevel);

            uint32 srcIdx = (i == 0)
                ? mDownChain[mIterations - 1].tex->GetImageIndex()
                : mUpChain[i - 1].tex->GetImageIndex();

            d.ExtValue[i] = Vec4(1.f / srcW, 1.f / srcH,
                                 static_cast<float>(srcIdx), 0.f);
        }
    }

    // COMPOSITE 
    {
        auto& d = dataTable[COMPOSITE_IDX];
        d.PreviousStep = static_cast<int32>(ToGBufferIndex(before));
        d.ExtTex[0]    = static_cast<int32>(mUpChain.back().tex->GetImageIndex());
        d.ExtValue[0]  = Vec4(mIntensity, 0.f, 0.f, 0.f);
    }
}

void DualKawaseBlurPass::Execute(std::vector<DrawBatch>& /*batches*/)
{
    if (!mEnabled) return;

    RunExtract();
    RunDownsample();
    RunUpsample();
    RunComposite();

    // GlobalParams.etc, casdcae 원복 (다른 패스에 영향 없도록)
    uint32 zero = 0;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 1); // etc
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2); // casdcae
}


// RunExtract  — HDR 씬에서 임계값 이상 밝기 추출
void DualKawaseBlurPass::RunExtract()
{
    // mBefore RT: PIXEL_SHADER_RESOURCE 상태 (이전 패스에서 WaitTargetToResource 완료)
    // mExtractRT: COMMON -> RENDER_TARGET
    TransitionTo(mExtractRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_RENDER_TARGET);

    ClearAndBindRTV(mExtractRT);
    SetViewportAndScissor(mExtractRT.width, mExtractRT.height);

    uint32 passIdx = EXTRACT_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    RESOURCEMANAGER.Get<Shader>(L"KawaseExtract")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    // RENDER_TARGET -> PIXEL_SHADER_RESOURCE (다음 단계가 샘플링)
    TransitionTo(mExtractRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


// RunDownsample  — N회 Dual Kawase 다운샘플
void DualKawaseBlurPass::RunDownsample()
{
    uint32 passIdx = DOWN_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    for (int i = 0; i < mIterations; ++i)
    {
        KawaseRT& dst = mDownChain[i];

        TransitionTo(dst.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_COMMON,
                     D3D12_RESOURCE_STATE_RENDER_TARGET);

        ClearAndBindRTV(dst);
        SetViewportAndScissor(dst.width, dst.height);

        // GlobalParams.etc = step 인덱스 (HLSL이 ExtValue[etc]에서 texelSize, srcIdx 조회)
        uint32 step = static_cast<uint32>(i);
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &step, 1);

        RESOURCEMANAGER.Get<Shader>(L"KawaseDown")->Update();
        RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

        TransitionTo(dst.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

// RunUpsample  — N회 Dual Kawase 업샘플
void DualKawaseBlurPass::RunUpsample()
{
    uint32 passIdx = UP_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    for (int i = 0; i < mIterations; ++i)
    {
        KawaseRT& dst = mUpChain[i];

        TransitionTo(dst.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_COMMON,
                     D3D12_RESOURCE_STATE_RENDER_TARGET);

        ClearAndBindRTV(dst);
        SetViewportAndScissor(dst.width, dst.height);

        uint32 step = static_cast<uint32>(i);
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &step, 1);

        RESOURCEMANAGER.Get<Shader>(L"KawaseUp")->Update();
        RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

        TransitionTo(dst.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}


// RunComposite  — 블러 결과를 mAfter RT에 씬 + 이미시브로 합성
void DualKawaseBlurPass::RunComposite()
{
    RestoreViewportAndScissor();

    // mAfter RT에 출력 (PostProcessPass가 ping-pong 관리)
    RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint8>(mAfter)).ClearRenderTargetView();
    RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint8>(mAfter)).OMSetRenderTargets();

    uint32 passIdx = COMPOSITE_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    RESOURCEMANAGER.Get<Shader>(L"KawaseComposite")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint8>(mAfter)).WaitTargetToResource();

    // 업샘플 RT들은 다음 프레임 Initialize 전까지 PIXEL_SHADER_RESOURCE로 유지
    // (재사용을 위해 COMMON으로 복귀)
    for (auto& rt : mUpChain)
    {
        TransitionTo(rt.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                     D3D12_RESOURCE_STATE_COMMON);
    }
    for (auto& rt : mDownChain)
    {
        TransitionTo(rt.tex->GetTex2D().Get(),
                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                     D3D12_RESOURCE_STATE_COMMON);
    }
    TransitionTo(mExtractRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_COMMON);
}


KawaseRT DualKawaseBlurPass::CreateKawaseRT(const wstring& name, uint32 w, uint32 h)
{
    KawaseRT result;
    result.width  = w;
    result.height = h;

    // 텍스처 생성 (createSRVUAV = true → TextureMaps[] 자동 SRV 등록)
    result.tex = RESOURCEMANAGER.CreateTexture(
        name,
        DXGI_FORMAT_R16G16B16A16_FLOAT, w, h,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        true);

    // RTV 직접 할당
    uint32 rtvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    uint32 rtvIdx  = RENDERMANAGER.GetRenderTargetHeap()->GetRtvIndex();

    result.rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        RENDERMANAGER.GetRenderTargetHeap()->GetRtvHeapBegin(),
        rtvIdx * rtvSize);

    DEVICE->CreateRenderTargetView(result.tex->GetTex2D().Get(), nullptr, result.rtv);
    RENDERMANAGER.GetRenderTargetHeap()->SetRtvIndex(rtvIdx + 1);

    return result;
}

void DualKawaseBlurPass::SetViewportAndScissor(uint32 w, uint32 h)
{
    D3D12_VIEWPORT vp = { 0.f, 0.f, static_cast<float>(w), static_cast<float>(h), 0.f, 1.f };
    D3D12_RECT     sc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
    GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
    GRAPHICS_CMD_LIST->RSSetScissorRects(1, &sc);
}

void DualKawaseBlurPass::RestoreViewportAndScissor()
{
    GRAPHICS_CMD_LIST->RSSetViewports(1, &RENDERMANAGER.GetViewPort());
    GRAPHICS_CMD_LIST->RSSetScissorRects(1, &RENDERMANAGER.GetScissorRect());
}

void DualKawaseBlurPass::TransitionTo(
    ID3D12Resource*       res,
    D3D12_RESOURCE_STATES from,
    D3D12_RESOURCE_STATES to)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(res, from, to);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
}

void DualKawaseBlurPass::ClearAndBindRTV(const KawaseRT& rt)
{
    const float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
    GRAPHICS_CMD_LIST->ClearRenderTargetView(rt.rtv, clearColor, 0, nullptr);
    GRAPHICS_CMD_LIST->OMSetRenderTargets(1, &rt.rtv, false, nullptr);
}
