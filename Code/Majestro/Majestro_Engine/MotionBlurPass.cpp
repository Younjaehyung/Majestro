#include "pch.h"
#include "MotionBlurPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void MotionBlurPass::Initialize()
{
}

void MotionBlurPass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	// HDR 입력 RT 인덱스
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].PreviousStep
		= static_cast<int32>(ToGBufferIndex(before));
	// ExtTex[0]: MOTION_VECTOR RT 인덱스 (PS에서 velocity 읽기용)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtTex[0]
		= static_cast<int32>(GBUFFER_INDEX::GBUFFER_MOTIONVEC_INDEX);
}

void MotionBlurPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled) return;

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitResourceToTarget();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

	RESOURCEMANAGER.Get<Shader>(L"MotionBlur")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();
}
