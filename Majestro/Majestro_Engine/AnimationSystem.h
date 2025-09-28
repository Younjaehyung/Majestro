#pragma once
#include "World.h"
#include "System.h"

struct AnimationPass{
	int32	FrameCount{};
	double	Duration{};

	double	StartTime{};
	double	EndTime{};
};

class AnimationSystem : public System
{
public:
	AnimationSystem(World* world);
	void Initialize();

	void Update(float);

private:	// COMPUTE 애니메이션 시스템

	void ClearVector();

	void AnimationPush(float);
	void AnimationCompute();
	void AnimationBlend(float);

	
	void AnimationDispatch();


private:	// CPU 애니메이션 시스템


private:
	vector<struct KeyFrameInfo> mAniKeyFrame;
	vector<struct AnimationClipMeta> mAniClipMeta;
	vector<Matrix> mBoneData;
	vector<AnimationPass> mAnimationPass;

	shared_ptr<class Shader> mAnimationShader;
};
