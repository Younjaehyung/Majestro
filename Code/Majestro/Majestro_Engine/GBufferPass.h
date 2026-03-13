#pragma once
#include "RenderPass.h"
#include "RenderSystem.h"

class GBufferPass : public RenderPass
{
public:
  GBufferPass() = default;
  ~GBufferPass() = default;

	void Initialize();
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

	void InstancingRender(DrawBatch& drawBatch);
private:
	uint32 mCurrPSOID{};

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

