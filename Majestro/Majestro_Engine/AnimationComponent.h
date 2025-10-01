#pragma once
#include "Component.h"
#include "Skeleton.h"
#include "Animator.h"

class Material;
class StructuredBuffer;


struct AnimationInstance {
	uint32	SkeletonID;     // 스켈레톤 핸들(Initialize에서 부여한 값)
	uint32	AnimClipIdx{};
	uint32	CurrentFrame{};
	uint32	NextFrame{};
	float	Ratio{};

	uint32	BoneCount{};
	uint32	ReulstIndex{};
};


struct AnimatorParams
{
	uint32 skeletonID;   // 스켈레톤 핸들(Initialize에서 Skeleton::SetSkeletonHandle로 지정한 값)
	uint32 clipA;        // 현재 재생 클립(2-way 블렌드라면 B 포함)
	uint32 clipB;
	uint32 flags;        // 루프/루트모션/애디티브 등

	float  timeA;        // seconds
	float  timeB;
	float  weightB;      // 0~1
	float  playbackRate; // 1.0 = normal

	uint32 outBaseIndex; // ★ CS가 쓰고 VS가 읽는 팔레트 시작 오프셋
};

class AnimationComponent : public Component<AnimationComponent>
{
public:
	AnimationComponent();
	AnimationComponent(vector<shared_ptr<Animator>>& AnimClips);

public:

	vector<shared_ptr<Animator>>	mAnimClips;
	uint32							mSkeletonHandle;	// if Skeleton Enable use this Handle

	float							mUpdateTime = 0.f;
	int32							mClipIndex = 0;
	int32							mFrame = 0;
	int32							mNextFrame = 0;
	uint32							mBoneCount = 0;	
	uint32							ReulstIndex{};

	float							mFrameRatio = 0;

	bool							mBoneFinalUpdated = false;

	uint32							mStructuredBufferIndex=0;
};

