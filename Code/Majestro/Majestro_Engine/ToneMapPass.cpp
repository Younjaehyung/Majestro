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
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_TONEMAP_PASS)].PreviousStep = static_cast<int32>(ToGBufferIndex(before));
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
