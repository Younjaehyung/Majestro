#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class ToneMapPass : public RenderPass
{
public:
	ToneMapPass() = default;
	~ToneMapPass() = default;

	virtual void Initialize();
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after);

	// HDR RT → SwapChain RT (ToneMap 셰이더)
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

	void SetColorGrading(const ColorGradingParams& params) { mColorGrading = params; }
	const ColorGradingParams& GetColorGrading() const { return mColorGrading; }

private:
	ColorGradingParams mColorGrading;
};

