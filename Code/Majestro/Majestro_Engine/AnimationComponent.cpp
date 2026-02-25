#include "pch.h"
#include "AnimationComponent.h"

AnimationComponent::AnimationComponent()
{
}

AnimationComponent::AnimationComponent(vector<shared_ptr<Animator>>& AnimClips)
{
	mAnimClips = AnimClips;
	mAnimInstance.SkeletonID = mAnimClips[0]->GetSkeleton()->GetSkeletonHandle();

	mAnimInstance.BoneCount = static_cast<uint32>(mAnimClips[0]->GetSkeleton()->GetBones().size());
	if (mAnimInstance.BoneCount > 0)
	{
		mUpperBlendMaskStart = min(mUpperBlendMaskStart, mAnimInstance.BoneCount - 1);
		mUpperBlendMaskEnd = mAnimInstance.BoneCount - 1;
	}
}
