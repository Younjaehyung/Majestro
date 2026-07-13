#include "pch.h"
#include "ToneMapPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Texture.h"

#include "RenderSystem.h"

void ToneMapPass::Initialize()
{
}

void ToneMapPass::SetColorLUT(const std::wstring& name, int size, float strength)
{
	mLutName = name; mLutSize = size; mLutStrength = strength;
}

void ToneMapPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	auto& entry = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_TONEMAP_PASS)];
	entry.PreviousStep = static_cast<int32>(ToGBufferIndex(before));

	// 컬러 그레이딩 파라미터 업로드
	//   ExtValue[0] = (Saturation, Contrast, Brightness, Exposure)
	//   Exposure: 톤매핑 전 HDR 배율. 1.0 = 기본, >1 = 밝게, <1 = 어둡게

	entry.ExtValue[0] = Vec4(
		mColorGrading.Saturation,
		mColorGrading.Contrast,
		mColorGrading.Brightness,
		mColorGrading.Exposure);

	// ExtValue[1] = (ShadowTint.rgb, ShadowStrength)
	entry.ExtValue[1] = Vec4(
		mColorGrading.ShadowTint.x,
		mColorGrading.ShadowTint.y,
		mColorGrading.ShadowTint.z,
		mColorGrading.ShadowStrength);

	// ExtValue[2] = (MidtoneTint.rgb, MidtoneStrength)
	entry.ExtValue[2] = Vec4(
		mColorGrading.MidtoneTint.x,
		mColorGrading.MidtoneTint.y,
		mColorGrading.MidtoneTint.z,
		mColorGrading.MidtoneStrength);

	// ExtValue[3] = (HighlightTint.rgb, HighlightStrength)
	entry.ExtValue[3] = Vec4(
		mColorGrading.HighlightTint.x,
		mColorGrading.HighlightTint.y,
		mColorGrading.HighlightTint.z,
		mColorGrading.HighlightStrength);

	// LUT 텍스처와 파라미터 전달
	auto lut = mLutName.empty() ? nullptr : RESOURCEMANAGER.Get<Texture>(mLutName);
	entry.ExtTex[0] = lut ? static_cast<int32>(lut->GetImageIndex()) : -1;
	entry.ExtTex[1] = mLutSize;
	entry.ExtTex[2] = (int)(std::clamp(mLutStrength, 0.f, 1.f) * 100); // 강도 0~100
	// LUT 텍스처 크기 전달
	entry.ExtTex[3] = lut ? static_cast<int32>(lut->GetWidth()) : 0;
	entry.ExtTex[4] = lut ? static_cast<int32>(lut->GetHeight()) : 0;
	entry.ExtTex[5] = mColorGradingEnabled ? 1 : 0;
	
}

void ToneMapPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	auto& outputGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter));

	
	outputGroup.WaitResourceToTarget();
	outputGroup.OMSetRenderTargets();

	RESOURCEMANAGER.Get<Shader>(L"ToneMap")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	outputGroup.WaitTargetToResource();
}

