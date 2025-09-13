#include "pch.h"
#include "AnimationSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "Timer.h"
#include "AnimationComponent.h"

AnimationSystem::AnimationSystem(World* world) : System::System(world)
{
}

void AnimationSystem::Initialize() 
{
	// 불변 데이터  update
	//RENDERMANAGER.GetGroupBuffer( FRAMERESOURCEIDNEX )->AnimationInfo->PushGraphicsData(mObjectVector.data(), static_cast<uint32>(sizeof(objectParams) * mObjectVector.size()));
}


void AnimationSystem::Update(float deltaTime)
{
	ClearVector();
	AnimationBlend(deltaTime);
	AnimationUpdate(deltaTime);
	AnimationCompute();
}

void AnimationSystem::ClearVector()
{
	mAnimationVector.clear();
}

void AnimationSystem::AnimationBlend(float deltaTime)
{
}

void AnimationSystem::AnimationUpdate(float deltaTime)
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






		animCom->mStructuredBufferIndex = static_cast<uint32>(mAnimationVector.size());

		mAnimationVector.insert(mAnimationVector.end(), animCom->mBoneFinalMatrix.begin(), animCom->mBoneFinalMatrix.end());
		
		
	}
	
}

