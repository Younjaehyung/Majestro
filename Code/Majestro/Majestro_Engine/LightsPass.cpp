#include "pch.h"
#include "LightsPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void LightsPass::Initialize() {
}

void LightsPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {
	
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).OMSetRenderTargets();

    for (auto& light : deferredDrawBatchs) {

        light.PSOShader->Update();

        light.Mesh->Render();
    }
    ////리소스에서 타켓으로
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).WaitTargetToResource();
}