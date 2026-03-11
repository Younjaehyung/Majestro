#include "pch.h"
#include "ForwardPass.h"


#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void ForwardPass::Initialize() {
}

void ForwardPass::DispatchForwardPlusCull() {
    const uint32 tileCountX = (RENDERMANAGER.GetWindow().Width + FORWARD_PLUS_TILE_SIZE - 1) / FORWARD_PLUS_TILE_SIZE;
    const uint32 tileCountY = (RENDERMANAGER.GetWindow().Height + FORWARD_PLUS_TILE_SIZE - 1) / FORWARD_PLUS_TILE_SIZE;

    auto groupBuffer = RENDERMANAGER.GetGroupBuffer(mFrameIndex);
    auto tileMeta = groupBuffer->ForwardPlusTileMetaInfo;
    auto lightIndices = groupBuffer->ForwardPlusLightIndexInfo;

    auto tileMetaResource = tileMeta->GetBuffer().Get();
    auto lightIndexResource = lightIndices->GetBuffer().Get();

    auto toTileMetaUav = CD3DX12_RESOURCE_BARRIER::Transition(tileMetaResource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    auto toLightIndexUav = CD3DX12_RESOURCE_BARRIER::Transition(lightIndexResource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    COMPUTE_CMD_LIST->ResourceBarrier(1, &toTileMetaUav);
    COMPUTE_CMD_LIST->ResourceBarrier(1, &toLightIndexUav);

    COMPUTE_CMD_LIST->SetComputeRootSignature(RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature")->GetRootSignature().Get());
    ID3D12DescriptorHeap* descHeap = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap().Get();
    COMPUTE_CMD_LIST->SetDescriptorHeaps(1, &descHeap);
    RENDERMANAGER.SetComputTable();

    auto cullShader = RESOURCEMANAGER.Get<Shader>(L"ForwardPlusCull");
    COMPUTE_CMD_LIST->SetPipelineState(cullShader->GetPipelineState().Get());

    dum.BaseInstance = tileCountX;
    dum.InstanceCount = tileCountY;
    dum.Cascade = FORWARD_PLUS_MAX_LIGHTS_PER_TILE;
    COMPUTE_CMD_LIST->SetComputeRoot32BitConstants(0, 3, &(dum), 0);
    COMPUTE_CMD_LIST->Dispatch(tileCountX, tileCountY, 1);

    auto uavBarrier0 = CD3DX12_RESOURCE_BARRIER::UAV(tileMetaResource);
    auto uavBarrier1 = CD3DX12_RESOURCE_BARRIER::UAV(lightIndexResource);
    COMPUTE_CMD_LIST->ResourceBarrier(1, &uavBarrier0);
    COMPUTE_CMD_LIST->ResourceBarrier(1, &uavBarrier1);

    auto toTileMetaSrv = CD3DX12_RESOURCE_BARRIER::Transition(tileMetaResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON);

    auto toLightIndexSrv = CD3DX12_RESOURCE_BARRIER::Transition(lightIndexResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON);

    COMPUTE_CMD_LIST->ResourceBarrier(1, &toTileMetaSrv);
    COMPUTE_CMD_LIST->ResourceBarrier(1, &toLightIndexSrv);
}


void ForwardPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {
	dum = { 0, 0, 0 };
    mCurrPSOID = 0;
    mFrameIndex = RENDERMANAGER.GetFrameResourceIndex();

    // DispatchForwardPlusCull();
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

    hdrGroup.WaitResourceToTarget();
    hdrGroup.OMSetRenderTargetsReadOnlyDepth(); // read-only DSV로 depth test 유지

    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD)
            continue;

        if (mCurrPSOID != drawBatch.PSOID) {
            drawBatch.PSOShader->Update();
            mCurrPSOID = drawBatch.PSOID;
        }
        dum.BaseInstance = drawBatch.BaseInstance;
        dum.InstanceCount = drawBatch.InstanceCount;

        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &(dum), 0);
        InstancingRender(drawBatch);
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
    DispatchForwardPlusCull();
}

void ForwardPass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}
