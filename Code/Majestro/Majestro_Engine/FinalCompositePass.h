#pragma once
#include "RenderTarget.h"
#include "RenderPass.h"

class FinalCompositePass : public RenderPass
{
public:
	FinalCompositePass() = default;
	~FinalCompositePass() = default;
	// HDR RT + GBuffer RT → SwapChain RT (합성 셰이더)
	void Initialize();
	void SetBlur(bool on);
	void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	void Execute(std::vector<DrawBatch>& deferredDrawBatchs);
private:
	bool mBlurEnabled = false;
};

