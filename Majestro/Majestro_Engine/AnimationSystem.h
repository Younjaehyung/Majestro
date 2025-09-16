#pragma once
#include "World.h"
#include "System.h"

struct AnimationPass{
	int32			frameCount{};
	double			duration{};

	double mStartTime{};
	double mEndTime{};
};

class AnimationSystem : public System
{
public:
	AnimationSystem(World* world);
	void Initialize();

	void Update(float);

private:	// COMPUTE 애니메이션 시스템

	void ClearVector();

	void AnimationBlend(float);
	void AnimationPush(float);
	void AnimationCompute();
private:	// CPU 애니메이션 시스템


private:
	vector<struct KeyFrameInfo> mAniKeyFrame;
	vector<struct AnimationClipMeta> mAniClipMeta;
	vector<AnimationPass> mAnimationPass;
};
