#include "pch.h"
#include "DepthPrePass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void DepthPrePass::Initialize() {
	depthShader      = RESOURCEMANAGER.Get<Shader>(L"DepthPrepass");
	depthShaderAlpha = RESOURCEMANAGER.Get<Shader>(L"DepthPrepassAlpha");
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
    // (이후 G_BUFFER, ForwardPass 등이 같은 depth texture로 depth test 사용)
    depthGroup.ClearRenderTargetView();
    depthGroup.OMSetRenderTargets();

    depthShader->Update();
    mCurrPSOID = 0;

    // 1) 일반 불투명 오브젝트 depth 기록
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

    // 2) 알파 컷아웃 식생 depth 기록 (FogPass가 올바른 depth를 읽도록)
    depthShaderAlpha->Update();
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

void DepthPrePass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex, 0, 0);
}
