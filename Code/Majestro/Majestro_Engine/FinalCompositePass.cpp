#include "pch.h"
#include "FinalCompositePass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

#include "ToneMapPass.h"
#include "ChromaticAberrationPass.h"

void FinalCompositePass::Initialize() 
{

}

void FinalCompositePass::SetBlurEnabled(bool on)
{
	mBlurEnabled = on;
}

void FinalCompositePass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::COMPOSITE_PASS)].PreviousStep = static_cast<int32>(ToGBufferIndex(before));
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::COMPOSITE_PASS)].ExtValue[0] = Vec4(float(mBlurEnabled), 1.0f, 1.0f, 1.0f);
}

void FinalCompositePass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
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
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1,backIndex);
	}

	// HDR RT는 EffectPass에서 WaitTargetToResource()로 SRV 상태
	RESOURCEMANAGER.Get<Shader>(L"FianlComposite")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	if (RENDERMANAGER.IsMsaaEnabled())
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();

}
