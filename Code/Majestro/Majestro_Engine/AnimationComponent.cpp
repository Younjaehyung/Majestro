#include "pch.h"
#include "AnimationComponent.h"

AnimationComponent::AnimationComponent()
{
}

AnimationComponent::AnimationComponent(vector<shared_ptr<Animator>>& AnimClips)
{
	mAnimClips = AnimClips;
	mAnimInstance.SkeletonID = mAnimClips[0]->GetSkeleton()->GetSkeletonHandle();

	shared_ptr<Skeleton> skeleton = mAnimClips[0]->GetSkeleton();
	mAnimInstance.BoneCount = static_cast<uint32>(skeleton->GetBones().size());
	if (mAnimInstance.BoneCount > 0)
	{
		// 스켈레톤 자동 탐지한 상체 범위
		skeleton->BuildUpperBodyMaskRange();
		mUpperBlendMaskStart = min(skeleton->mSpineBoneStartOffset, mAnimInstance.BoneCount - 1);
		mUpperBlendMaskEnd = min(skeleton->mSpineBoneEndOffset, mAnimInstance.BoneCount - 1);
	}
}
