#pragma once
#include "RenderSystem.h"

class ShadowPass
{
public:
  ShadowPass() = default;
  ~ShadowPass() = default;

	void Initialize();
	void Update(std::vector<DrawBatch>& deferredDrawBatchs, array<bool, 4>& cascadeActive);

	void RenderShadowCamera(std::vector<DrawBatch>& deferredDrawBatchs, uint32 cascadeIndex);

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

