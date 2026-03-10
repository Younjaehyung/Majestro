#pragma once
#include "RenderTarget.h"

class FinalCompositePass
{
public:
	FinalCompositePass() = default;
	~FinalCompositePass() = default;
	// HDR RT + GBuffer RT → SwapChain RT (합성 셰이더)
	void Execute(RENDER_TARGET_GROUP_TYPE);
};

