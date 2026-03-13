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
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs, std::vector<DrawBatch>& shadowOnlyBatchs, array<bool, 4>& cascadeActive);

	void RenderShadowCamera(std::vector<DrawBatch>& drawBatchs, uint32 cascadeIndex);

	void InstancingRender(DrawBatch& drawBatch);
private:

	array<Matrix, 4> mCascadeView{};
	array<Matrix, 4> mCascadeProjection{};

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

