#pragma once

class ToneMapPass
{
public:
	ToneMapPass() = default;
	~ToneMapPass() = default;

	// HDR RT → SwapChain RT (ToneMap 셰이더)
	void Execute();
};
