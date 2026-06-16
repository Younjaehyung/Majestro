#include "pch.h"
#include "HealthVignettePass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Timer.h"

void HealthVignettePass::Initialize()
{
}

void HealthVignettePass::SetFeedbackState(float lowHealthStrength, float healStrength)
{
	mLowHealthStrength = std::clamp(lowHealthStrength, 0.0f, 1.0f);
	mHealStrength = std::clamp(healStrength, 0.0f, 1.0f);
}

void HealthVignettePass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	auto& d = dataTable[HEALTH_VIGNETTE_IDX];
	d.PreviousStep = static_cast<int32>(ToGBufferIndex(before));
	d.ExtTex[0] = -1;
	d.ExtTex[1] = -1;

	if (mNoiseTextureName.empty() == false)
	{
		shared_ptr<Texture> noiseTexture = RESOURCEMANAGER.Get<Texture>(mNoiseTextureName);
		if (noiseTexture != nullptr)
		{
			// 비네팅 얼룩 노이즈 텍스처 슬롯
			d.ExtTex[0] = static_cast<int32>(noiseTexture->GetImageIndex());
		}
	}
	// x  : 시간 y :  빈사 강도 z  : 회복 강도 w : 노이즈 텍스처 타일링
	d.ExtValue[0] = Vec4(TIMER.GetTotalTime(), mLowHealthStrength, mHealStrength, 2.25f);

	// x,y,z : 빈사 비네팅 색 w: 맥동 속도
	d.ExtValue[1] = Vec4(1.0f, 0.045f, 0.02f, 4.5f);

	// x,y,z : 회복 비네팅 w: 색 맥동 속도
	d.ExtValue[2] = Vec4(0.35f, 1.0f, 0.08f, 3.0f);

	// 외곽 마스크와 일렁임 모양 조정 값
	// x : 시작 반경 y : 최대 반경 z :  일렁임 크기 w : 일렁임 빈도
	d.ExtValue[3] = Vec4(0.42f, 0.90f, 0.26f, 18.0f);
}

void HealthVignettePass::Execute(std::vector<DrawBatch>& /*deferredDrawBatchs*/)
{
	if (mEnabled == false)
		return;

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).ClearRenderTargetView();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).OMSetRenderTargets();

	uint32 passIdx = HEALTH_VIGNETTE_IDX;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passIdx, 3);

	RESOURCEMANAGER.Get<Shader>(L"HealthVignette")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).WaitTargetToResource();
}
