#include "pch.h"
#include "GBufferPass.h"


#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void GBufferPass::Initialize() {
}

void GBufferPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).OMSetRenderTargets();
    mCurrPSOID = 0;
    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::DEFERRED)
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

    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).WaitTargetToResource();
}
