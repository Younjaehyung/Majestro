#include "pch.h"
#include "DepthPrePass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void DepthPrePass::Initialize() {
	mDepthShader      = RESOURCEMANAGER.Get<Shader>(L"DepthPrepass");
	mDepthAlphaShader = RESOURCEMANAGER.Get<Shader>(L"DepthPrepassAlpha");
}

void DepthPrePass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
	mAfter = after;

}

void DepthPrePass::Execute(vector<DrawBatch>& drawBatchs) {
    auto& depthGroup = RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint32>(mAfter));

    // ClearRenderTargetView 내부에서 배리어 + DSV 클리어 처리
    // 렌더 후 WaitTargetToResource 호출 안 함 → DEPTH_WRITE 상태 유지

    depthGroup.ClearRenderTargetView();
    depthGroup.OMSetRenderTargets();

   
    mCurrPSOID = 0;

    // 일반 불투명 오브젝트 depth 기록
    mDepthShader->Update();
    for (auto& batch : drawBatchs) {
        const SHADER_TYPE type = batch.PSOShader->GetShaderType();
        if (type != SHADER_TYPE::DEFERRED && type != SHADER_TYPE::FORWARD)
            continue;

        const BLEND_TYPE blend = batch.PSOShader->GetBlendType();
        if (blend == BLEND_TYPE::ALPHA_BLEND || blend == BLEND_TYPE::ONE_TO_ONE_BLEND
            || blend == BLEND_TYPE::ALPHA_TEST)
            continue;

        dum.BaseInstance  = batch.BaseInstance;
        dum.InstanceCount = batch.InstanceCount;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &dum, 0);
        InstancingRender(batch);
    }

    // alpha 있는 depth 기록
    mDepthAlphaShader->Update();
    for (auto& batch : drawBatchs) {
        if (batch.PSOShader->GetBlendType() != BLEND_TYPE::ALPHA_TEST)
            continue;

        dum.BaseInstance  = batch.BaseInstance;
        dum.InstanceCount = batch.InstanceCount;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &dum, 0);
        InstancingRender(batch);
    }
    // depth texture는 DEPTH_WRITE 상태 유지 (G_BUFFER, Forward에서 depth test 가능)
}
