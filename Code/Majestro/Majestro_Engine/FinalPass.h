#pragma once
#include "RenderPass.h"

class FinalPass : public RenderPass
{
public:
	FinalPass() = default;
	~FinalPass() = default;
	void Initialize();
	//void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	void Update();
};

