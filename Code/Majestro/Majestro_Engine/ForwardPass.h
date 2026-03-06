#pragma once
#include "RenderSystem.h"

class ForwardPass
{
public:
	ForwardPass() = default;
	~ForwardPass() = default;

	void Initialize();
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

	void InstancingRender(DrawBatch& drawBatch);
private:
	void DispatchForwardPlusCull();
	void SetComputeTableOnGraphicsCmdList();

private:
	uint32 mCurrPSOID{};
	uint32 mFrameIndex{};

	static constexpr uint32 FORWARD_PLUS_TILE_SIZE = 16;
	static constexpr uint32 FORWARD_PLUS_MAX_LIGHTS_PER_TILE = 128;

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

