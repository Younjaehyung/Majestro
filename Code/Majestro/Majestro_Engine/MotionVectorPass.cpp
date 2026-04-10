#include "pch.h"
#include "MotionVectorPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void MotionVectorPass::Initialize()
{
}

void MotionVectorPass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	// PRE_DEPTH 값 읽기
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::POST_MOTIONVEC_PASS)].PreviousStep
		= static_cast<int32>(ToGBufferIndex(RENDER_TARGET_GROUP_TYPE::PRE_DEPTH));
}

void MotionVectorPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled) return;

	auto& mvGroup = RENDERMANAGER.GetRenderTargetGroup(
		static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::MOTION_VECTOR));

	
	mvGroup.ClearRenderTargetView();
	mvGroup.OMSetRenderTargets();

	RESOURCEMANAGER.Get<Shader>(L"MotionVector")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	mvGroup.WaitTargetToResource();
}
