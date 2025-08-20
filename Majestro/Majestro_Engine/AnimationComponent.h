#pragma once
#include "Component.h"

class BoneInfo;
class AnimClipInfo;
class Material;
class BoneInfo;
class StructuredBuffer;

class AnimationComponent : public Component<AnimationComponent>
{

private:
	const vector<BoneInfo>* mBones;
	const vector<AnimClipInfo>* mAnimClips;
public:
	float							mUpdateTime = 0.f;
	int32							mClipIndex = 0;
	int32							mFrame = 0;
	int32							mNextFrame = 0;
	float							mFrameRatio = 0;

	shared_ptr<Material>			mComputeMaterial;
	shared_ptr<StructuredBuffer>	mBoneFinalMatrix; 
	bool							mBoneFinalUpdated = false;
};

