#pragma once
#include "RenderSystem.h"

class ForwardPass
{
public:
	ForwardPass() = default;
	~ForwardPass() = default;

	void Initialize();
	void Update(std::vector<DrawBatch>& deferredDrawBatchs);

	void InstancingRender(DrawBatch& drawBatch);
private:
	uint32 mCurrPSOID{};

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

