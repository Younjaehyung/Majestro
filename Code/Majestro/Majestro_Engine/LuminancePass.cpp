#include "pch.h"
#include "LuminancePass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void LuminancePass::Initialize()
{
}

void LuminancePass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	// HDR 입력 RT 인덱스
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_LUMINANCE_PASS)].PreviousStep
		= static_cast<int32>(ToGBufferIndex(before));
	// ExtTex[0]: MOTION_VECTOR RT 인덱스 (PS에서 velocity 읽기용)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_LUMINANCE_PASS)].ExtTex[0]
		= static_cast<uint32>(ToGBufferIndex(before));

	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_LUMINANCE_PASS)].ExtTex[1]
		= RESOURCEMANAGER.Get<Texture>(L"GradientTex")->GetImageIndex();
}

void LuminancePass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled) return;

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitResourceToTarget();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

	uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_LUMINANCE_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &(passCustomIdx), 3);
	RESOURCEMANAGER.Get<Shader>(L"Luminance")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();
}
