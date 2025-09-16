#pragma once
#include "Component.h"
#include "Skeleton.h"
#include "Animator.h"

class Material;
class StructuredBuffer;

class AnimationComponent : public Component<AnimationComponent>
{
public:
	//void Play();

public:
	const vector<Skeleton>*			mBones;
	const vector<Animator>*			mAnimClips;

	uint32							mSkeletonHandle;	// if Skeleton Enable use this Handle
	float							mUpdateTime = 0.f;
	int32							mClipIndex = 0;
	int32							mFrame = 0;
	int32							mNextFrame = 0;
	float							mFrameRatio = 0;

	vector<Matrix>					mBoneFinalMatrix;
	bool							mBoneFinalUpdated = false;

	uint32							mStructuredBufferIndex=0;


	vector<Matrix>	mOffsetBuffer; // 각 뼈의 offset 정렬			(불변/ bone값)
	vector<Matrix>	mFrameBuffer; // 전체 본 프레임 정보		(가변/ finalupdate값)

};

