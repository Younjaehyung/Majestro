#include "pch.h"
#include "AnimationComponent.h"

AnimationComponent::AnimationComponent()
{
}

AnimationComponent::AnimationComponent(vector<shared_ptr<Animator>>& AnimClips)
{
	mAnimClips = AnimClips;
	mSkeletonHandle = mAnimClips[0]->GetSkeleton()->GetSkeletonHandle();
	mBoneCount = mAnimClips[0]->GetSkeleton()->GetBones().size();
}
