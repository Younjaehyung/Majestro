#include "pch.h"
#include "FXAAPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void FXAAPass::Initialize()
{

}

void FXAAPass::SetParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality)
{
    mEdgeThreshold    = edgeThreshold;
    mEdgeThresholdMin = edgeThresholdMin;
    mSubpixQuality    = subpixQuality;
}

void FXAAPass::SetData(
    std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before,
    RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter  = after;

    auto& d       = dataTable[FXAA_IDX];
   
    d.PreviousStep = static_cast<int32>(ToGBufferIndex(before));
  
    d.ExtValue[0]  = Vec4(mEdgeThreshold, mEdgeThresholdMin, mSubpixQuality, 0.0f);
}

void FXAAPass::Execute(std::vector<DrawBatch>& /*deferredDrawBatchs*/)
{
    if (!mEnabled) return;

   
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).ClearRenderTargetView();
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).OMSetRenderTargets();

    
    uint32 passIdx = FXAA_IDX;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

   
    RESOURCEMANAGER.Get<Shader>(L"FXAA")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

  
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).WaitTargetToResource();
}
