#pragma once
#include "RenderSystem.h"

class ShadowPass
{
public:
  ShadowPass() = default;
  ~ShadowPass() = default;

	void Initialize();
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs, std::vector<DrawBatch>& shadowOnlyBatchs, array<bool, 4>& cascadeActive);

	void RenderShadowCamera(std::vector<DrawBatch>& drawBatchs, uint32 cascadeIndex);

	void InstancingRender(DrawBatch& drawBatch);
private:

	array<Matrix, 4> mCascadeView{};
	array<Matrix, 4> mCascadeProjection{};

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

