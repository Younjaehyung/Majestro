#pragma once
#include "RenderPass.h"

class LobbyBackgroundPass : public RenderPass
{
public:
	void Initialize() override;
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;
};
