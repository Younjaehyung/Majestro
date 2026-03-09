#pragma once
#include "RenderTarget.h"


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

  void AddPass(shared_ptr<PostProcess> pass) { mPasses.push_back(pass); }

private:
	RENDER_TARGET_GROUP_TYPE mBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mAfterGroupType;
	vector<shared_ptr<PostProcess>> mPasses;


};

