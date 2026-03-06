#pragma once
#include "RenderSystem.h"

class LightsPass
{
public:
  LightsPass() = default;
  ~LightsPass() = default;

  void Initialize();
  void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

};

