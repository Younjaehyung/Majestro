#pragma once
#include "RenderTarget.h"

class ToneMapPass;
class FinalCompositePass;


class PostProcess
{
public:
	PostProcess() = default;
	virtual ~PostProcess() = default;
	virtual void Initialize();

	virtual void Execute(RENDER_TARGET_GROUP_TYPE , RENDER_TARGET_GROUP_TYPE);

	virtual bool IsEnabled() const { return mEnabled; }
	virtual void SetEnabled(bool enabled) { mEnabled = enabled; }
protected:

	bool mEnabled = true;
};

class PostProcessPass
{
public:
	PostProcessPass() = default;
	~PostProcessPass() = default;
  
  void Initialize();
  void Execute();

  void AddHDRPass(shared_ptr<PostProcess> pass) { mHDRPasses.push_back(pass); }
  void AddLDRPass(shared_ptr<PostProcess> pass) { mLDRPasses.push_back(pass); }

private:
	RENDER_TARGET_GROUP_TYPE mLDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mLDRAfterGroupType;

	RENDER_TARGET_GROUP_TYPE mHDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mHDRAfterGroupType;


	vector<shared_ptr<PostProcess>> mHDRPasses;
	vector<shared_ptr<PostProcess>> mLDRPasses;

	std::shared_ptr<ToneMapPass> mToneMapPass;
	std::shared_ptr<FinalCompositePass> mFinalCompositePass;
};

