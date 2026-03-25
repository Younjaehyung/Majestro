#include "pch.h"
#include "ShadowPass.h"


#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void ShadowPass::Initialize() {}

void ShadowPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
	mAfter = after;

}

void ShadowPass::Execute(std::array<std::vector<DrawBatch>, 4>& cascadeDrawBatchs, array<bool, 4>& cascadeActive)
{
    auto& shadowGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW));

    // ClearRenderTargetView 내부에서 WaitResourceToTarget(PSR→DEPTH_WRITE)을 처리
    shadowGroup.ClearRenderTargetView();  // 모든 cascade slice 클리어

    for (uint32 cascadeIndex = 0; cascadeIndex < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++cascadeIndex) {
        if (!cascadeActive[cascadeIndex])
            continue;

        // 각 cascade에 해당하는 DSV slice를 바인딩
        shadowGroup.OMSetRenderTargets(0, cascadeIndex);

        // 해당 cascade 구체와 교차하는 오브젝트만 렌더 (퍼-캐스케이드 컬링)
        RenderShadowCamera(cascadeDrawBatchs[cascadeIndex], cascadeIndex);
    }

    shadowGroup.WaitTargetToResource();
}

void ShadowPass::RenderShadowCamera(std::vector<DrawBatch>& deferredDrawBatchs, uint32 cascadeIndex) 
{
    shared_ptr<Shader> defaultShadowShader = RESOURCEMANAGER.Get<Shader>(L"Shadow");
    shared_ptr<Shader> terrainShadowShader = RESOURCEMANAGER.Get<Shader>(L"TerrainShadow");
    shared_ptr<Shader> terrainShader = RESOURCEMANAGER.Get<Shader>(L"Terrain");
    int32 lastShadowShader = -1;
    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::DEFERRED &&
            drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD) {
            continue;
        }

        const int32 shadowShaderType = (drawBatch.PSOShader == terrainShader) ? 1 : 0;
        if (shadowShaderType != lastShadowShader) {
            if (shadowShaderType == 1)
                continue;
            else
                defaultShadowShader->Update();

            lastShadowShader = shadowShaderType;
        }

        dum.BaseInstance = drawBatch.BaseInstance;
        dum.InstanceCount = drawBatch.InstanceCount;
        dum.Cascade = cascadeIndex;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &(dum), 0);
        InstancingRender(drawBatch);
    }
}

void ShadowPass::InstancingRender(DrawBatch& drawBatch) 
{
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}