#include "pch.h"
#include "GodRayPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void GodRayPass::Initialize()
{
}

void GodRayPass::SetData(
    std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter  = after;

    auto& d = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_GODRAY_PASS)];

    // 이전 패스 HDR RT (씬 컬러 입력)
    d.PreviousStep = static_cast<int32>(ToGBufferIndex(before));


    Vec3 normalizedDir = mSunDir;
    normalizedDir.Normalize();

    d.ExtValue[0] = Vec4(normalizedDir.x, normalizedDir.y, normalizedDir.z, mIntensity);
    d.ExtValue[1] = Vec4((float)mNumSteps, mMaxRayLen, mScatterCoeff, mMieG);
    d.ExtValue[2] = Vec4(mSunColor.x, mSunColor.y, mSunColor.z, mAbsorptionCoeff);
}

void GodRayPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
    if (!mEnabled) return;

    // ─── Depth 배리어: DEPTH_WRITE -> PIXEL_SHADER_RESOURCE ─────────────────
    // VLS PS에서 Gbuffer[0](SceneDepth)을 SRV로 읽기 위해 상태 전환
    auto& hdrGroup    = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
    auto  depthTex    = hdrGroup.GetDSTexture();
    auto* depthRes    = depthTex->GetTex2D().Get();

    auto toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        depthRes,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toSRV);

    // 렌더 타깃 설정 
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

    uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_GODRAY_PASS);
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passCustomIdx, 3);

    RESOURCEMANAGER.Get<Shader>(L"GodRay")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();

    // Depth 배리어 복구: PIXEL_SHADER_RESOURCE  > DEPTH_WRITE
    auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
        depthRes,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
}
