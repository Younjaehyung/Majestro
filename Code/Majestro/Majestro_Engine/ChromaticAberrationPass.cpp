#include "pch.h"
#include "ChromaticAberrationPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void ChromaticAberrationPass::Initialize()
{
}

void ChromaticAberrationPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_CA_PASS)].PreviousStep = static_cast<int32>(ToGBufferIndex(before));
}

void ChromaticAberrationPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (mEnabled == false)
		return;

	auto& outputGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(mAfter));

	outputGroup.ClearRenderTargetView();
	outputGroup.OMSetRenderTargets();

	RESOURCEMANAGER.Get<Shader>(L"ChromaticAberration")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	outputGroup.WaitTargetToResource();
}
