#include "pch.h"
#include "FogPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void FogPass::Initialize()
{
}

void FogPass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	// HDR 입력 RT 인덱스
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FOG_PASS)].PreviousStep
		= static_cast<int32>(ToGBufferIndex(before));

	// ExtValue[0]: FogColor (R, G, B, unused) - 밝은 하늘빛 회색
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FOG_PASS)].ExtValue[0]
		= Vec4(0.75f, 0.80f, 0.88f, 0.0f);
	// ExtValue[1]: fogStart, fogEnd, fogHeightStart, fogHeightEnd
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FOG_PASS)].ExtValue[1]
		= Vec4(7000.f, 15000.0f, -5.0f, 15.0f);
	// ExtValue[2]: heightFogWeight (0이면 height fog 비활성)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FOG_PASS)].ExtValue[2]
		= Vec4(0.0f, 0.0f, 0.0f, 0.0f);

}

void FogPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled) return;

	// ClearRenderTargetView()가 내부적으로 WaitResourceToTarget(COMMON→RT)을 처리하므로 별도 호출 불필요
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

	uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FOG_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &(passCustomIdx), 3);
	RESOURCEMANAGER.Get<Shader>(L"Fog")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();
}
