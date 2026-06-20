#pragma once
#include "RenderPass.h"

class World;

class LobbyBackgroundPass : public RenderPass
{
public:
	void Initialize(World* world);
	void SetData(
		std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before,
		RENDER_TARGET_GROUP_TYPE after) override;
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;

private:
	World* mWorld = nullptr;
};
