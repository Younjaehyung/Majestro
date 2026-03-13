#pragma once
#include "RenderPass.h"
#include "RenderSystem.h"

class LightsPass : public RenderPass
{
public:
  LightsPass() = default;
  ~LightsPass() = default;

  void Initialize();
  void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

};

