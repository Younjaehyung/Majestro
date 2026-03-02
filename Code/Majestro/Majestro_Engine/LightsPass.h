#pragma once
#include "RenderSystem.h"

class LightsPass
{
public:
  LightsPass() = default;
  ~LightsPass() = default;

  void Initialize();
  void Update(std::vector<DrawBatch>& deferredDrawBatchs);

};

