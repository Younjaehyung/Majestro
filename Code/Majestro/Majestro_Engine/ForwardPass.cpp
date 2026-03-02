#include "pch.h"
#include "ForwardPass.h"


#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void ForwardPass::Initialize() {
}

void ForwardPass::Update(std::vector<DrawBatch>& deferredDrawBatchs) {
	dum = { 0, 0, 0 };
    mCurrPSOID = 0;
    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
    if (RENDERMANAGER.IsMsaaEnabled()) { // msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitResourceToTarget();
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
    }
    else {
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
    }

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
    if (RENDERMANAGER.IsMsaaEnabled()) { // msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();
    }
}

void ForwardPass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}
