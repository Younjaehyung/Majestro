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
	//if (mAnimInstance.BoneCount > 0)
	//{
	//	// 기본 마스크 시작점을 "실질 상체"(blendWeight가 충분히 큰 첫 본)로 맞춘다.
	//	// pelvis(0.5) 같은 경계 본이 Upper 레이어 영향을 받아 하체 방향이 끌려가지 않도록 한다.
	//	const auto& bones = mAnimClips[0]->GetSkeleton()->GetBones();
	//	uint32 detectedUpperStart = 0;
	//	for (uint32 i = 0; i < static_cast<uint32>(bones.size()); ++i)
	//	{
	//		if (bones[i].blendWeight >= 0.7f)
	//		{
	//			detectedUpperStart = i;
	//			break;
	//		}
	//	}

	//	mUpperBlendMaskStart = max(mUpperBlendMaskStart, detectedUpperStart);
	//	mUpperBlendMaskStart = min(mUpperBlendMaskStart, mAnimInstance.BoneCount - 1);
	//	mUpperBlendMaskEnd = mAnimInstance.BoneCount - 1;
	//}
}
