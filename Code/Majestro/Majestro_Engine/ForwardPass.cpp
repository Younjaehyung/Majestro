#include "pch.h"
#include "ForwardPass.h"


#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void ForwardPass::Initialize() {
}

void ForwardPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
	mAfter = after;

}

void ForwardPass::SetEmissiveTarget(RENDER_TARGET_GROUP_TYPE group, uint32 index)
{
    mEmissiveGroup     = group;
    mEmissiveIndex     = index;
    mHasEmissiveTarget = true;
}

void ForwardPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {
	mDum = { 0, 0, 0 };
    mCurrPSOID = 0;

    auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));

    // ─── Depth 상태 전환: DEPTH_WRITE → DEPTH_READ | PIXEL_SHADER_RESOURCE ──────
    // ForwardPass PS에서 Gbuffer[0](depthPreTexture)을 SRV로 읽어 Rim Light를 계산.
    // DEPTH_READ 상태에서도 read-only DSV를 통해 depth test 수행 가능.
    auto  depthTex      = hdrGroup.GetDSTexture();
    auto* depthResource = depthTex->GetTex2D().Get();

    auto toDepthRead = CD3DX12_RESOURCE_BARRIER::Transition(
        depthResource,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthRead);

    // 이미시브 블룸 소스 RT 준비
    ID3D12Resource*             emissiveResource = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE emissiveRTV{};

    if (mHasEmissiveTarget)
    {
        auto& emissiveGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mEmissiveGroup));
        emissiveResource = emissiveGroup.GetRTTexture(static_cast<uint8>(mEmissiveIndex))->GetTex2D().Get();
        emissiveRTV      = emissiveGroup.GetRTVHandle(mEmissiveIndex);


        auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            emissiveResource,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        GRAPHICS_CMD_LIST->ResourceBarrier(1, &toRenderTarget);
    }

    hdrGroup.WaitResourceToTarget();
    // read-only DSV로 depth test 유지 + 이미시브 RT를 SV_Target1로 이어 붙임
    if (emissiveResource)
        hdrGroup.OMSetRenderTargetsReadOnlyDepth(&emissiveRTV, 1);
    else
        hdrGroup.OMSetRenderTargetsReadOnlyDepth();

    // ForwardPass 전용 PassCustomTable 행 인덱스 설정 (DWORD[3] 단독 업데이트)
    const uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::FORWARD_PASS);
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passCustomIdx, 3);

    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD)
            continue;

        if (mCurrPSOID != drawBatch.PSOID) {
            drawBatch.PSOShader->Update();
            mCurrPSOID = drawBatch.PSOID;
        }
        mDum.BaseInstance = drawBatch.BaseInstance;
        mDum.InstanceCount = drawBatch.InstanceCount;

        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &(mDum), 0);
        InstancingRender(drawBatch);
    }


    if (emissiveResource)
    {
        auto toCommon = CD3DX12_RESOURCE_BARRIER::Transition(
            emissiveResource,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_COMMON);
        GRAPHICS_CMD_LIST->ResourceBarrier(1, &toCommon);
    }

    hdrGroup.WaitTargetToResource();

    // ─── Depth 상태 복구: DEPTH_READ | PIXEL_SHADER_RESOURCE → DEPTH_WRITE ──────
    // 이후 패스(EffectPass 등)가 depth write를 사용할 수 있도록 복구.
    auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
        depthResource,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
}

void ForwardPass::Compute()
{
#if USE_CPU_ANIMATION
    // Forward Plus 제거 후 이 패스에서 제출할 Compute 명령은 없다.
    RENDERMANAGER.SetAnimationComputeFenceValue(0);
#endif
}
