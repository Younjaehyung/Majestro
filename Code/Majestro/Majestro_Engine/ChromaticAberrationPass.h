#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class ChromaticAberrationPass : public RenderPass
{
public:
	ChromaticAberrationPass() = default;
	virtual ~ChromaticAberrationPass() = default;

	virtual void Initialize();
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after);
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

};

