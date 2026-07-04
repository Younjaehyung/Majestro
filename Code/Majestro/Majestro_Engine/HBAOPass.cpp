#include "pch.h"
#include "HBAOPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"

void HBAOPass::Initialize()
{
    uint32 sw = RENDERMANAGER.GetRenderWidth();
    uint32 sh = RENDERMANAGER.GetRenderHeight();

    if (sw == 0u || sh == 0u) { 
        sw = static_cast<uint32>(RENDERMANAGER.GetViewPort().Width); 
        sh = static_cast<uint32>(RENDERMANAGER.GetViewPort().Height); 
    }

    // 절반 해상도 AO
    uint32 hw = max(sw / 2u, 1u);
    uint32 hh = max(sh / 2u, 1u);

    mAoRT   = CreateHbaoRT(L"HbaoAoRT",   hw, hh, DXGI_FORMAT_R8_UNORM);
    mBlurRT = CreateHbaoRT(L"HbaoBlurRT", hw, hh, DXGI_FORMAT_R8_UNORM);
}

void HBAOPass::SetData(
    std::array<PassCustomData,
    static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before,
    RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter  = after;

    // texel = AO 렌더타깃(절반 해상도) 기준 — HBAO 스크린 반경/블러 스텝이 RT 픽셀 단위라 RT 크기와 일치해야 함
    float texelW = 1.0f / static_cast<float>(mAoRT.width);
    float texelH = 1.0f / static_cast<float>(mAoRT.height);

   
    // ExtValue[0] = (radius, bias, intensity, numSteps)
    // ExtValue[1] = (numDirs, texelW, texelH, falloff)
    {
        auto& d = dataTable[HBAO_IDX];
        d.ExtValue[0] = Vec4(mRadius, mBias, mIntensity, static_cast<float>(mNumSteps));
        d.ExtValue[1] = Vec4(static_cast<float>(mNumDirections), texelW, texelH, mFalloff);
    }

   
    // ExtValue[0] = 가로 blur (etc=0): (texelW, texelH, rawAoIdx, blurRadius)
    // ExtValue[1] = 세로 blur (etc=1): (texelW, texelH, blurRtIdx, blurRadius)
    {
        auto& d = dataTable[BLUR_IDX];
        d.ExtValue[0] = Vec4(texelW, texelH,
                             static_cast<float>(mAoRT.tex->GetImageIndex()),
                             mBlurRadius);
        d.ExtValue[1] = Vec4(texelW, texelH,
                             static_cast<float>(mBlurRT.tex->GetImageIndex()),
                             mBlurRadius);
    }

    
    // mEnabled=false 이면 -1 로 설정 , lighting 셰이더에서 ao=1.0 fallback
    {
        auto& d = dataTable[LIGHTS_IDX];
        d.ExtTex[0] = mEnabled ? static_cast<int32>(mAoRT.tex->GetImageIndex()): -1;
    }
}

void HBAOPass::Execute(std::vector<DrawBatch>& /*batches*/)
{
    if (!mEnabled) return;

    RunHBAO();
    RunBlurH();
    RunBlurV();

    // 절반 해상도로 렌더했으니 뷰포트를 풀해상도로 복구 (다음 LightsPass가 전체 화면 렌더)
    RestoreViewportAndScissor();
}




void HBAOPass::RunHBAO() // mAoRT : G-Buffer(Position, Normal) 에서 원시 AO 값 계산
{
    // 이전 프레임 LightsPass 가 샘플링한 뒤 PSR 상태일 수 있으므로 분기
    D3D12_RESOURCE_STATES prevState = mAoRTInPSR
        ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        : D3D12_RESOURCE_STATE_COMMON;
    TransitionTo(mAoRT.tex->GetTex2D().Get(),
                 prevState,
                 D3D12_RESOURCE_STATE_RENDER_TARGET);
    mAoRTInPSR = false;

    ClearAndBindRTV(mAoRT, 1.0f); // AO 기본값 = 1 (완전 비차폐)
    SetViewportAndScissor(mAoRT.width, mAoRT.height);

    uint32 passIdx = HBAO_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    RESOURCEMANAGER.Get<Shader>(L"HBAO")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    TransitionTo(mAoRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


void HBAOPass::RunBlurH() // mBlurRT : 가로 Cross-bilateral blur: mAoRT(PSR)
{
    TransitionTo(mBlurRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_RENDER_TARGET);

    ClearAndBindRTV(mBlurRT, 1.0f);
    SetViewportAndScissor(mBlurRT.width, mBlurRT.height);

    uint32 passIdx = BLUR_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    uint32 dir = 0; // 가로
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &dir, 1); // etc=0

    RESOURCEMANAGER.Get<Shader>(L"HBAOBlur")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    TransitionTo(mBlurRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void HBAOPass::RunBlurV() // mAoRT : 세로 Cross-bilateral blur: mBlurRT(PSR)
{
    // mAoRT: PSR -> RT (Raw HBAO 결과 덮어쓰기)
    TransitionTo(mAoRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_RENDER_TARGET);

    ClearAndBindRTV(mAoRT, 1.0f);
    SetViewportAndScissor(mAoRT.width, mAoRT.height);

    uint32 passIdx = BLUR_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

    uint32 dir = 1; // 세로
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &dir, 1); // etc=1

    RESOURCEMANAGER.Get<Shader>(L"HBAOBlur")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    // 최종 AO -> PSR (LightsPass 에서 TextureMaps 로 샘플링)
    TransitionTo(mAoRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    mAoRTInPSR = true; // 다음 프레임 RunHBAO 에서 PSR→RT 전환


    TransitionTo(mBlurRT.tex->GetTex2D().Get(),
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_COMMON);
}


HbaoRT HBAOPass::CreateHbaoRT(const wstring& name, uint32 w, uint32 h, DXGI_FORMAT fmt)
{
    HbaoRT result;
    result.width  = w;
    result.height = h;


    result.tex = RESOURCEMANAGER.CreateTexture(
        name,
        fmt, w, h,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        true,
        1,
        0,
        Vec4(1.f, 1.f, 1.f, 1.f));

    // RTV 수동 할당
    uint32 rtvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    uint32 rtvIdx  = RENDERMANAGER.GetRenderTargetHeap()->GetRtvIndex();

    result.rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        RENDERMANAGER.GetRenderTargetHeap()->GetRtvHeapBegin(),
        rtvIdx * rtvSize);

    DEVICE->CreateRenderTargetView(result.tex->GetTex2D().Get(), nullptr, result.rtv);
    RENDERMANAGER.GetRenderTargetHeap()->SetRtvIndex(rtvIdx + 1);

    return result;
}

void HBAOPass::SetViewportAndScissor(uint32 w, uint32 h)
{
    D3D12_VIEWPORT vp = { 0.f, 0.f, static_cast<float>(w), static_cast<float>(h), 0.f, 1.f };
    D3D12_RECT     sc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
    GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
    GRAPHICS_CMD_LIST->RSSetScissorRects(1, &sc);
}

void HBAOPass::RestoreViewportAndScissor()
{
    GRAPHICS_CMD_LIST->RSSetViewports(1, &RENDERMANAGER.GetViewPort());
    GRAPHICS_CMD_LIST->RSSetScissorRects(1, &RENDERMANAGER.GetScissorRect());
}

void HBAOPass::TransitionTo(
    ID3D12Resource*       res,
    D3D12_RESOURCE_STATES from,
    D3D12_RESOURCE_STATES to)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(res, from, to);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
}

void HBAOPass::ClearAndBindRTV(const HbaoRT& rt, float clearVal)
{
    const float clearColor[4] = { clearVal, clearVal, clearVal, clearVal };
    GRAPHICS_CMD_LIST->ClearRenderTargetView(rt.rtv, clearColor, 0, nullptr);
    GRAPHICS_CMD_LIST->OMSetRenderTargets(1, &rt.rtv, false, nullptr);
}
