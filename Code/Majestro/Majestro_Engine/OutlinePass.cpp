#include "pch.h"
#include "OutlinePass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderSystem.h"

void OutlinePass::Initialize()
{
}

void OutlinePass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::FORWARD_PASS)].PreviousStep = static_cast<int32>(ToGBufferIndex(before));
	//dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::FORWARD_PASS)].ExtValue[0] = Vec4(0.0015f, 0.0f, 0.0f, 0.0f);
	dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::FORWARD_PASS)].ExtValue[0] = Vec4(0.0020f, 0.1f, 0.1f, 0.0f);
}

void OutlinePass::Execute(std::vector<DrawBatch>& drawBatches)
{
	auto outlineShader = RESOURCEMANAGER.Get<Shader>(L"Outline");
	if (!outlineShader)
		return;

	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(
		static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));

	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargets();

	outlineShader->Update();

	
	const uint32 passCustomIdx = static_cast<uint32>(PASS_CUSTOM_INDEX::FORWARD_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passCustomIdx, 3);

	for (auto& drawBatch : drawBatches)
	{
		// 카툰 셰이딩 오브젝트만 처리 (알파 컷아웃 식생 제외)
		if (drawBatch.PSOShader->GetShaderType() != SHADER_TYPE::FORWARD)
			continue;
		if (drawBatch.PSOShader->GetBlendType() == BLEND_TYPE::ALPHA_TEST)
			continue;

		mDum.BaseInstance  = drawBatch.BaseInstance;
		mDum.InstanceCount = drawBatch.InstanceCount;
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 3, &mDum, 0);

		InstancingRender(drawBatch);
	}

	hdrGroup.WaitTargetToResource();
}
