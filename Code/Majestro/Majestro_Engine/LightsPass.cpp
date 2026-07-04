#include "pch.h"
#include "LightsPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void LightsPass::Initialize() {
}

void LightsPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs) {

	// lighting_dir_PS가 Gbuffer[0](depthPre)을 SRV로 읽으므로 DEPTH_READ 상태 — read-only DSV로 바인딩
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).OMSetRenderTargetsReadOnlyDepth();

    // LIGHTS_PASS 슬롯에서 AO 텍스처 인덱스를 읽도록 PassCustomIndex 설정
    // lighting_dir_PS.hlsl 에서 TextureMaps[ExtTex[0]] 로 AO 샘플링
    uint32 lightsPassIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::LIGHTS_PASS);
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &lightsPassIdx, 3);

    for (auto& light : deferredDrawBatchs) {

        light.PSOShader->Update();

        light.Mesh->Render();
    }

    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).WaitTargetToResource();
}