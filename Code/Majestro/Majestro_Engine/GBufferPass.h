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


private:
	uint32 mCurrPSOID{};

};

