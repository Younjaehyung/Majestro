#include "pch.h"
#include "ToneMapPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

#include "RenderSystem.h"

void ToneMapPass::Initialize() {
}

void ToneMapPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	auto& entry = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_TONEMAP_PASS)];
	entry.PreviousStep = static_cast<int32>(ToGBufferIndex(before));

	// 컬러 그레이딩 파라미터 업로드
	// ExtValue[0] = (Saturation, Contrast, Brightness, Enabled)
	entry.ExtValue[0] = Vec4(
		mColorGrading.Saturation,
		mColorGrading.Contrast,
		mColorGrading.Brightness,
		mColorGrading.Enabled ? 1.0f : 0.0f);

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
}

void ToneMapPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

	if (RENDERMANAGER.IsMsaaEnabled())
	{
		auto& finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN));
		finalGroup.WaitResourceToTarget();
		finalGroup.OMSetRenderTargets(1, backIndex);
	}
	else
	{
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).WaitResourceToTarget();
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).OMSetRenderTargets();
		
	}

	// HDR RT는 EffectPass에서 WaitTargetToResource()로 SRV 상태
	RESOURCEMANAGER.Get<Shader>(L"ToneMap")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	if (RENDERMANAGER.IsMsaaEnabled())
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();
	else
		// MSAA 모드에서는 mAfter(POST_LDR_A)를 RT로 전환한 적 없으므로 WaitTargetToResource 호출 금지
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).WaitTargetToResource();

}
