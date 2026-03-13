#include "pch.h"
#include "ChromaticAberrationPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void ChromaticAberrationPass::Initialize() {

}

void ChromaticAberrationPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_CA_PASS)].PreviousStep = static_cast<int32>(before);

}

void ChromaticAberrationPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (mEnabled == false) return;

	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

	if (RENDERMANAGER.IsMsaaEnabled())
	{
		auto& finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN));
		finalGroup.WaitResourceToTarget();
		finalGroup.OMSetRenderTargets(1, backIndex);
	}
	else
	{
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).ClearRenderTargetView();
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).OMSetRenderTargets();
		// A-> B 입력 
	}

	// HDR RT는 EffectPass에서 WaitTargetToResource()로 SRV 상태
	RESOURCEMANAGER.Get<Shader>(L"ChromaticAberration")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	if (RENDERMANAGER.IsMsaaEnabled())
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();

	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter)).WaitTargetToResource();
}
