#include "pch.h"
#include "AnimationSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Timer.h"
#include "Animator.h"
#include "AnimationComponent.h"

AnimationSystem::AnimationSystem(World* world) : System::System(world)
{
}

void AnimationSystem::Initialize() 
{
	// 불변 데이터  update

	uint32 index{};
	for (auto& animClip : RESOURCEMANAGER.GetAllResources<Animator>()) {
		shared_ptr<Animator> aniClip = dynamic_pointer_cast<Animator>(animClip.second);

		for (const auto& v : aniClip->keyFrames) {
			mAniKeyFrame.insert(mAniKeyFrame.end(), v.begin(), v.end());
		}
		mAniClipMeta.push_back(aniClip->mClipMeta);
		index++;
	}

	RENDERMANAGER.GetAnimationBuffers()->AnimationMeta ->PushDefaultToData(mAniKeyFrame.data(), static_cast<uint32>(mAniKeyFrame.size() * sizeof(KeyFrameInfo)));
	RENDERMANAGER.GetAnimationBuffers()->AnimationClip ->PushDefaultToData(mAniClipMeta.data(), static_cast<uint32>(mAniClipMeta.size() * sizeof(AnimationClipMeta)));
}


void AnimationSystem::Update(float deltaTime)
{
	ClearVector();
	AnimationBlend(deltaTime);
	AnimationPush(deltaTime);
	AnimationCompute();
}

void AnimationSystem::ClearVector()
{
}

void AnimationSystem::AnimationBlend(float deltaTime)
{


}

void AnimationSystem::AnimationPush(float deltaTime)
{
	vector<Entity> animationsEntity = mWorld->GetEntitiesWithComponent<AnimationComponent>();
	for (auto& entity : animationsEntity) {
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
		animCom->mBoneFinalMatrix.clear();

		animCom->mUpdateTime += deltaTime;
		const Animator& animClip = animCom->mAnimClips->at(animCom->mClipIndex);

		if (animCom->mUpdateTime >= animClip.duration)
			animCom->mUpdateTime = 0.f;

		const int32 ratio = static_cast<int32>(animClip.frameCount / animClip.duration);
		animCom->mFrame = static_cast<int32>(animCom->mUpdateTime * ratio);
		animCom->mFrame = min(animCom->mFrame, animClip.frameCount - 1);
		animCom->mNextFrame = min(animCom->mFrame + 1, animClip.frameCount - 1);
		animCom->mFrameRatio = static_cast<float>(animCom->mUpdateTime - animCom->mFrame);

		//animCom->mBoneFinalMatrix.push_back();

		animCom->mStructuredBufferIndex = static_cast<uint32>(mAnimationPass.size());

		//mAnimationPass.insert(mAnimationPass.end(), animCom->mBoneFinalMatrix.begin(), animCom->mBoneFinalMatrix.end());
		
		
	}
	
}

void AnimationSystem::AnimationCompute()
{
	uint32 boneCount = 64;	// TEMP
	uint32 groupCount = (boneCount / 256) + 1;
	COMPUTE_CMD_LIST->Dispatch(groupCount, 1, 1);
}

