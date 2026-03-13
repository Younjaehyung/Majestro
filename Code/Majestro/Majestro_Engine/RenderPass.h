#pragma once
#include "RenderTarget.h"
#include "RenderSystem.h"
class ToneMapPass;
class FinalCompositePass;


class RenderPass
{
public:
	RenderPass() = default;
	virtual ~RenderPass() = default;
	virtual void Initialize();
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, 
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after);
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

	virtual bool IsEnabled() const { return mEnabled; }
	virtual void SetEnabled(bool enabled) { mEnabled = enabled; }
protected:
	RENDER_TARGET_GROUP_TYPE mBefore;
	RENDER_TARGET_GROUP_TYPE mAfter;


	bool mEnabled = true;
};

class PostProcessPass
{
public:
	PostProcessPass() = default;
	~PostProcessPass() = default;
  
  void Initialize();
  void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>&);
  void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

  void AddHDRPass(shared_ptr<RenderPass> pass) { mHDRPasses.push_back(pass); }
  void AddLDRPass(shared_ptr<RenderPass> pass) { mLDRPasses.push_back(pass); }

private:
	RENDER_TARGET_GROUP_TYPE mLDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mLDRAfterGroupType;

	RENDER_TARGET_GROUP_TYPE mHDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mHDRAfterGroupType;


	vector<shared_ptr<RenderPass>> mHDRPasses;
	vector<shared_ptr<RenderPass>> mLDRPasses;

	std::shared_ptr<ToneMapPass> mToneMapPass;
	std::shared_ptr<FinalCompositePass> mFinalCompositePass;
};

