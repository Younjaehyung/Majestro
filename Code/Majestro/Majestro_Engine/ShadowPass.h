#pragma once
#include "RenderSystem.h"

class ShadowPass
{
public:
  ShadowPass() = default;
  ~ShadowPass() = default;

	void Initialize();
	void Update(std::vector<DrawBatch>& deferredDrawBatchs);

	void RenderShadowCamera(std::vector<DrawBatch>& deferredDrawBatchs, uint32 cascadeIndex);

	void InstancingRender(DrawBatch& drawBatch);
private:
	array<bool, 4> mCascadeActive = { true, true, true, true };
	array<float, 4> CascadeSplit = { 150.f, 436.f, 596.f, 1500.f };
	array<Matrix, 4> mCascadeView{};
	array<Matrix, 4> mCascadeProjection{};

	struct dummy {
		uint32 BaseInstance;
		uint32 InstanceCount;
		uint32 Cascade;
	} dum;
};

