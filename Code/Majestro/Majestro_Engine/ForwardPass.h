#pragma once
#include "RenderSystem.h"
#include "RenderPass.h"

class ForwardPass : public RenderPass
{
public:
	ForwardPass() = default;
	~ForwardPass() = default;

	void Initialize();
	void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs);
	void Compute();
private:
	void DispatchForwardPlusCull();

private:
	uint32 mCurrPSOID{};
	uint32 mFrameIndex{};

	static constexpr uint32 FORWARD_PLUS_TILE_SIZE = 16;
	static constexpr uint32 FORWARD_PLUS_MAX_LIGHTS_PER_TILE = 128;

};

