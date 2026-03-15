#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class MotionVectorPass : public RenderPass
{
public:
	MotionVectorPass() = default;
	virtual ~MotionVectorPass() = default;

	virtual void Initialize() override;
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;
};
