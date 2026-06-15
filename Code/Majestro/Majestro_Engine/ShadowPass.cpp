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

void ShadowPass::Execute(std::array<std::vector<DrawBatch>, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>& cascadeDrawBatchs, array<bool, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>& cascadeActive, bool& mapCascadeDirty)
{
    auto& shadowGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW));
    shadowGroup.TransitionToTarget();  // PSR -> DEPTH_WRITE (첫 프레임은 생략)

    for (uint32 cascadeIndex = 0; cascadeIndex < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++cascadeIndex) {
        const bool isMapCascade = (cascadeIndex == SHADOW_MAP_CASCADE_INDEX);

        if (!cascadeActive[cascadeIndex]) {
            // 비활성 이동 cascade 클리어
            if (!isMapCascade)
                shadowGroup.ClearRenderTargetView(cascadeIndex);
            continue;
        }


        // 맵 고정 cascade: dirty가 아니면 이전 프레임 depth 유지 (캐쉬)
        if (isMapCascade && !mapCascadeDirty)
            continue;

        shadowGroup.ClearRenderTargetView(cascadeIndex);  // 해당 슬라이스만 클리어
        shadowGroup.OMSetRenderTargets(0, cascadeIndex);

        RenderShadowCamera(cascadeDrawBatchs[cascadeIndex], cascadeIndex);
    }

    mapCascadeDirty = false;  // 소비 — 다음 무효화까지 맵 슬라이스 재사용
    shadowGroup.WaitTargetToResource();
}

void ShadowPass::RenderShadowCamera(std::vector<DrawBatch>& deferredDrawBatchs, uint32 cascadeIndex)
{
    shared_ptr<Shader> defaultShadowShader = RESOURCEMANAGER.Get<Shader>(L"Shadow");
    shared_ptr<Shader> alphaShadowShader   = RESOURCEMANAGER.Get<Shader>(L"ShadowAlpha");

    // 일반 불투명 오브젝트
    defaultShadowShader->Update();
    for (auto& drawBatch : deferredDrawBatchs) {
        const SHADER_TYPE type = drawBatch.PSOShader->GetShaderType();
        if (type != SHADER_TYPE::DEFERRED && type != SHADER_TYPE::FORWARD)
            continue;
        if (drawBatch.PSOShader->GetBlendType() == BLEND_TYPE::ALPHA_TEST)
            continue;

        mDum.BaseInstance  = drawBatch.BaseInstance;
        mDum.InstanceCount = drawBatch.InstanceCount;
        mDum.Cascade       = cascadeIndex;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &mDum, 0);
        InstancingRender(drawBatch);
    }

    // 알파 컷아웃 식생 — 텍스처 알파로 clip해 올바른 그림자 실루엣
    alphaShadowShader->Update();
    for (auto& drawBatch : deferredDrawBatchs) {
        if (drawBatch.PSOShader->GetBlendType() != BLEND_TYPE::ALPHA_TEST)
            continue;

        mDum.BaseInstance  = drawBatch.BaseInstance;
        mDum.InstanceCount = drawBatch.InstanceCount;
        mDum.Cascade       = cascadeIndex;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &mDum, 0);
        InstancingRender(drawBatch);
    }
}
