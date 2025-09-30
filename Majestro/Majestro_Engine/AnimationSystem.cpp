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

	uint32 index1{};
	for (auto& animClip : RESOURCEMANAGER.GetAllResources<Animator>()) {
		shared_ptr<Animator> aniClip = dynamic_pointer_cast<Animator>(animClip.second);

		for (const auto& v : aniClip->GetKeyFrames()) {
			mAniKeyFrame.insert(mAniKeyFrame.end(), v.begin(), v.end());
		}
		mAniClipMeta.push_back(aniClip->GetClipMeta());
		index1++;
	}
	
	uint32 index2{};
	for (auto& skels : RESOURCEMANAGER.GetAllResources<Skeleton>()) {
		shared_ptr<Skeleton> skel = dynamic_pointer_cast<Skeleton>(skels.second);

		for (auto& bone : skel->GetBones()) {
			mBoneData.emplace_back(bone.matOffset);	// why not .Transpose()?
		}
		
		skel->SetSkeletonHandle(index2++);
	}

	
	mAnimationShader = RESOURCEMANAGER.Get<Shader>(L"AnimationComputeShader");
	RENDERMANAGER.GetAnimationBuffers()->AnimationMeta ->PushDefaultToData(mAniClipMeta.data(), static_cast<uint32>(mAniClipMeta.size() * sizeof(AnimationClipMeta)));
	RENDERMANAGER.GetAnimationBuffers()->AnimationClip ->PushDefaultToData(mAniKeyFrame.data(), static_cast<uint32>(mAniKeyFrame.size() * sizeof(KeyFrameInfo)));
	RENDERMANAGER.GetAnimationBuffers()->SkeletonBone  ->PushDefaultToData(mBoneData.data(), static_cast<uint32>(mBoneData.size() * sizeof(Matrix)));
}


void AnimationSystem::Update(float deltaTime)
{
	RENDERMANAGER.SetComputTable();

	ClearVector();
	AnimationPush(deltaTime);
	GRAPHICS_CMD_LIST->SetPipelineState(mAnimationShader->GetPipelineState().Get());
	AnimationCompute();
	AnimationBlend(deltaTime);
}

void AnimationSystem::ClearVector()
{
    mAniKeyFrame.clear();
    mAniClipMeta.clear();
    mAnimationPass.clear();
}



void AnimationSystem::AnimationPush(float deltaTime)
{
	vector<Entity> animationsEntity = mWorld->GetEntitiesWithComponent<AnimationComponent>();
	for (auto& entity : animationsEntity) {
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
		//animCom->mBoneFinalMatrix.clear();

		animCom->mUpdateTime += deltaTime;
		shared_ptr<Animator> animClip = animCom->mAnimClips.at(animCom->mClipIndex);

		if (animCom->mUpdateTime >= animClip->mDuration)
			animCom->mUpdateTime = 0.f;

		const float ratio = static_cast<float>(animClip->mFrameCount / animClip->mDuration);
		animCom->mFrame = static_cast<int32>(animCom->mUpdateTime * ratio);
		animCom->mFrame = min(animCom->mFrame, animClip->mFrameCount - 1);
		animCom->mNextFrame = min(animCom->mFrame + 1, animClip->mFrameCount - 1);

		// 1. 현재 프레임의 시작 시간 계산
		float frameTime = animCom->mFrame / static_cast<float>(ratio);
		// 2. 다음 프레임의 시작 시간 계산
		float nextFrameTime = animCom->mNextFrame / static_cast<float>(ratio);
		// 3. 현재 시간에서 현재 프레임의 시작 시간을 빼고, 두 프레임 사이의 시간으로 나눔
		animCom->mFrameRatio = (animCom->mUpdateTime - frameTime) / (nextFrameTime - frameTime);

		//animCom->mBoneFinalMatrix.push_back();

		animCom->mStructuredBufferIndex = static_cast<uint32>(mAnimationPass.size());

		//mAnimationPass.insert(mAnimationPass.end(), animCom->mBoneFinalMatrix.begin(), animCom->mBoneFinalMatrix.end());

	}

}

void AnimationSystem::AnimationCompute()
{
	vector<Entity> animationsEntity = mWorld->GetEntitiesWithComponent<AnimationComponent>();
	struct dum {
		int32 ClipIndex;
		int32 CurrFrame;
		int32 NextFrame;
		float Ratio;

	} dummy;
	for (auto& entity : animationsEntity) {
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);

		dummy.ClipIndex = animCom->mClipIndex;
		dummy.CurrFrame = animCom->mFrame;
		dummy.NextFrame = animCom->mNextFrame;
		dummy.Ratio = animCom->mFrameRatio;

		GRAPHICS_CMD_LIST->SetComputeRoot32BitConstants(0, 4, &dummy, 0);
		AnimationDispatch();
	}

}




void AnimationSystem::AnimationBlend(float deltaTime)
{


}

void AnimationSystem::AnimationDispatch()
{
	auto* res = RENDERMANAGER.GetGroupBuffer(FRAMERESOURCEIDNEX)
		->AnimationInfo->GetBuffer().Get();

	auto b = CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	GRAPHICS_CMD_LIST->ResourceBarrier(1, &b);

	const uint32_t T = 256; // numthreads X
	uint32_t groupsX = (77 + T - 1) / T; // 올림

	GRAPHICS_CMD_LIST->Dispatch(groupsX, 1, 1);

	{
		auto uav = CD3DX12_RESOURCE_BARRIER::UAV(res);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &uav);
	}

	auto srv = CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON);

	GRAPHICS_CMD_LIST->ResourceBarrier(1, &srv);


}


