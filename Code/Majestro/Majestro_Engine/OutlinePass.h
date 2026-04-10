#pragma once
#include "RenderPass.h"

class OutlinePass : public RenderPass
{
public:
	OutlinePass() = default;
	~OutlinePass() = default;

	virtual void Initialize() override;
	void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	virtual void Execute(std::vector<DrawBatch>& drawBatches) override;

private:
};
