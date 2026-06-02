#include "pch.h"
#include "MotionBlurPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Timer.h"

void MotionBlurPass::Initialize()
{
}

void MotionBlurPass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	const uint32 passIndex = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS);

	dataTable[passIndex].PreviousStep = static_cast<int32>(ToGBufferIndex(before));
	dataTable[passIndex].ExtTex[0] = static_cast<int32>(GBUFFER_INDEX::GBUFFER_MOTIONVEC_INDEX);
	dataTable[passIndex].ExtTex[1] = RESOURCEMANAGER.Get<Texture>(L"NoiseTex")->GetSrvIndex();

	// 대시 속도 효과 조정 값 
	// y : 대시 블렌드
	// z : 속도선 발생 밀도
	// w : 속도선 밝기
	dataTable[passIndex].ExtValue[0] = Vec4(TIMER.GetTotalTime(), 1.0f, 0.72f, 1.15f);

	// 속도선 색
	dataTable[passIndex].ExtValue[1] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	dataTable[passIndex].ExtValue[2] = Vec4(TIMER.GetTotalTime(), 1.0f, 0.3f, 0.2f);
	dataTable[passIndex].ExtValue[3] = Vec4(TIMER.GetTotalTime(), 1.0f, 0.3f, 0.2f);
}

void MotionBlurPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled)
		return;

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

	uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &(passCustomIdx), 3);

	RESOURCEMANAGER.Get<Shader>(L"MotionBlur")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();
}
