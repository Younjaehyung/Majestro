#include "pch.h"
#include "ForwardPass.h"


#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void ForwardPass::Initialize() {
}

void ForwardPass::SetComputeTableOnGraphicsCmdList() {
    auto descHeap = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap();
    auto handle = descHeap->GetGPUDescriptorHandleForHeapStart();
    const uint32 handleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, GBUFFER_INDEX_START, handleSize));
    GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, GROUP_START + (mFrameIndex * GROUP_COUNT), handleSize));
    GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, PARTICLE_INDEX_START, handleSize));
    GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(4, CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, ANIMATION_INDEX_START, handleSize));
    GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(5, CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, TEXTURE_MATERIALS_INDEX_START, handleSize));
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

    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toTileMetaUav);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toLightIndexUav);

    GRAPHICS_CMD_LIST->SetComputeRootSignature(RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature")->GetRootSignature().Get());
    ID3D12DescriptorHeap* descHeap = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap().Get();
    GRAPHICS_CMD_LIST->SetDescriptorHeaps(1, &descHeap);
    SetComputeTableOnGraphicsCmdList();

    auto cullShader = RESOURCEMANAGER.Get<Shader>(L"ForwardPlusCull");
    GRAPHICS_CMD_LIST->SetPipelineState(cullShader->GetPipelineState().Get());

    dum.BaseInstance = tileCountX;
    dum.InstanceCount = tileCountY;
    dum.Cascade = FORWARD_PLUS_MAX_LIGHTS_PER_TILE;
    GRAPHICS_CMD_LIST->SetComputeRoot32BitConstants(0, 3, &(dum), 0);
    GRAPHICS_CMD_LIST->Dispatch(tileCountX, tileCountY, 1);

    auto uavBarrier0 = CD3DX12_RESOURCE_BARRIER::UAV(tileMetaResource);
    auto uavBarrier1 = CD3DX12_RESOURCE_BARRIER::UAV(lightIndexResource);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &uavBarrier0);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &uavBarrier1);

    auto toTileMetaSrv = CD3DX12_RESOURCE_BARRIER::Transition(tileMetaResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON);

    auto toLightIndexSrv = CD3DX12_RESOURCE_BARRIER::Transition(lightIndexResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON);

    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toTileMetaSrv);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toLightIndexSrv);
}


void ForwardPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {
	dum = { 0, 0, 0 };
    mCurrPSOID = 0;
    mFrameIndex = RENDERMANAGER.GetFrameResourceIndex();

    DispatchForwardPlusCull();
    auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
    hdrGroup.WaitResourceToTarget();
    hdrGroup.OMSetRenderTargets();

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
}

void ForwardPass::Compute()
{
    DispatchForwardPlusCull();
}

void ForwardPass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}
