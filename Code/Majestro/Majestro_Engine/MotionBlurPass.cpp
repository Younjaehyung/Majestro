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
	// HDR 입력 RT 인덱스
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].PreviousStep
		= static_cast<int32>(ToGBufferIndex(before));
	// ExtTex[0]: MOTION_VECTOR RT 인덱스 (PS에서 velocity 읽기용)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtTex[0]
		= static_cast<int32>(GBUFFER_INDEX::GBUFFER_MOTIONVEC_INDEX);
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtTex[1]
		= RESOURCEMANAGER.Get<Texture>(L"NoiseTex")->GetSrvIndex();

	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtValue[0]
		= Vec4(TIMER.GetTotalTime(), 0.2f, 0.08f, 0.15f); // ExtValue[0].x: 시간에 따른 노이즈 애니메이션, yzw: 임의의 노이즈 패턴 스케일과 강도 조절용 (현재는 고정값)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtValue[1]
		= Vec4(TIMER.GetTotalTime(), 0.2, 0.2, 0.2); // ExtValue[1].x: 시간에 따른 블러 애니메이션, yzw: 임의의 블러 스케일과 강도 조절용 (현재는 고정값)
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtValue[2]
		= Vec4(TIMER.GetTotalTime(), 1.0, 0.3, 0.2); 
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS)].ExtValue[3]
		= Vec4(TIMER.GetTotalTime(), 1.0, 0.3, 0.2);
}

void MotionBlurPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled) return;

	// ClearRenderTargetView()가 내부적으로 WaitResourceToTarget(COMMON→RT)을 처리하므로 별도 호출 불필요
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).OMSetRenderTargets();

	
	uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONBLUR_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &(passCustomIdx), 3);

	RESOURCEMANAGER.Get<Shader>(L"MotionBlur")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(mAfter)).WaitTargetToResource();
}
