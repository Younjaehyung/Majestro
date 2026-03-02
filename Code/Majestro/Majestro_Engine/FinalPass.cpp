#include "pch.h"
#include "FinalPass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"


void FinalPass::Initialize()
{
}

void FinalPass::Update()
{
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets();

    RESOURCEMANAGER.Get<Shader>(L"Final")->Update();

    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();
   /* for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::DEFERRED)
            continue;

        if (mCurrPSOID != drawBatch.PSOID) {
            drawBatch.PSOShader->Update();
            mCurrPSOID = drawBatch.PSOID;
        }
        dum.BaseInstance = drawBatch.BaseInstance;
        dum.InstanceCount = drawBatch.InstanceCount;

        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &(dum), 0);
        InstancingRender(drawBatch);
    }*/



	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).WaitTargetToResource();
}