#include "pch.h"
#include "DepthPrePass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void DepthPrePass::Initialize() {
	depthShader = RESOURCEMANAGER.Get<Shader>(L"DepthPrepass");
}

void DepthPrePass::Execute(vector<DrawBatch>& drawBatchs) {
    auto& depthGroup = RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::PRE_DEPTH));

    // ClearRenderTargetView 내부에서 배리어 + DSV 클리어 처리
    // 렌더 후 WaitTargetToResource 호출 안 함 → DEPTH_WRITE 상태 유지
    // (이후 G_BUFFER, ForwardPass 등이 같은 depth texture로 depth test 사용)
    depthGroup.ClearRenderTargetView();
    depthGroup.OMSetRenderTargets();

    depthShader->Update();
    mCurrPSOID = 0;

    for (auto& batch : drawBatchs) {
        const SHADER_TYPE type = batch.PSOShader->GetShaderType();
        if (type != SHADER_TYPE::DEFERRED && type != SHADER_TYPE::FORWARD)
            continue;

        const BLEND_TYPE blend = batch.PSOShader->GetBlendType();                 
        if (blend == BLEND_TYPE::ALPHA_BLEND || blend == BLEND_TYPE::ONE_TO_ONE_BLEND)
            continue;

        dum.BaseInstance = batch.BaseInstance;
        dum.InstanceCount = batch.InstanceCount;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &dum, 0);
        InstancingRender(batch);
    }
    // depth texture는 DEPTH_WRITE 상태 유지 (G_BUFFER, Forward에서 depth test 가능)
}

void DepthPrePass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex, 0, 0);
}
