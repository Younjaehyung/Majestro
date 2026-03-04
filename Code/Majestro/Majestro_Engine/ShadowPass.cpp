#include "pch.h"
#include "ShadowPass.h"


#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void ShadowPass::Initialize() {
}

void ShadowPass::Update(std::vector<DrawBatch>& deferredDrawBatchs, array<bool, 4>& cascadeActive) {

    auto& shadowGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW));
    shadowGroup.WaitResourceToTarget();

    shadowGroup.OMSetRenderTargets();

    shadowGroup.ClearRenderTargetView();
   /* for (auto& light : mWorld->GetEntitiesWithComponent<LightComponent>()) {

        if (lightComponent->mLightInfo.LightType !=
            static_cast<int32>(LIGHT_TYPE::DIRECTIONAL_LIGHT))
            continue;*/

        for (uint32 cascadeIndex = 0;
            cascadeIndex < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT;
            ++cascadeIndex) {
            if (!cascadeActive[cascadeIndex])
                continue;
        
            RenderShadowCamera(deferredDrawBatchs, cascadeIndex);
        }
    //}


    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).WaitTargetToResource();

}

void ShadowPass::RenderShadowCamera(std::vector<DrawBatch>& deferredDrawBatchs, uint32 cascadeIndex) {
    shared_ptr<Shader> defaultShadowShader = RESOURCEMANAGER.Get<Shader>(L"Shadow");
    shared_ptr<Shader> terrainShadowShader = RESOURCEMANAGER.Get<Shader>(L"TerrainShadow");
    shared_ptr<Shader> terrainShader = RESOURCEMANAGER.Get<Shader>(L"Terrain");
    int32 lastShadowShader = -1;
    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::DEFERRED &&
            drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD) {
            continue;
        }

        const int32 shadowShaderType =
            (drawBatch.PSOShader == terrainShader) ? 1 : 0;
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

void ShadowPass::InstancingRender(DrawBatch& drawBatch) {
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}