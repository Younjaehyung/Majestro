#pragma once
#include "RenderPass.h"
#include "RenderSystem.h"

class ShadowPass : public RenderPass
{
public:
  ShadowPass() = default;
  ~ShadowPass() = default;

	void Initialize();
	void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	void Execute(std::array<std::vector<DrawBatch>, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>& cascadeDrawBatchs, array<bool, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>& cascadeActive, bool& mapCascadeDirty);

	void RenderShadowCamera(std::vector<DrawBatch>& drawBatchs, uint32 cascadeIndex);
private:

	array<Matrix, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeView{};
	array<Matrix, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeProjection{};
};

