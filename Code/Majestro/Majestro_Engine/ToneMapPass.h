#pragma once
#include "PostProcessPass.h"
#include "RenderTarget.h"

class ToneMapPass : public PostProcess
{
public:
	ToneMapPass() = default;
	~ToneMapPass() = default;

	// HDR RT → SwapChain RT (ToneMap 셰이더)
	virtual void Execute(RENDER_TARGET_GROUP_TYPE, RENDER_TARGET_GROUP_TYPE);
};

