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

	mAnimationBuckets.reserve(16);
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
   //  mAniKeyFrame.clear();
   //  mAniClipMeta.clear();
    mAnimationPass.clear();
	mAnimationBuckets.clear();
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

	
		mAnimationPass.emplace_back(animCom->mSkeletonHandle,animCom->mClipIndex, animCom->mFrame, animCom->mNextFrame, animCom->mFrameRatio,animCom->mBoneCount,0);

	}

	std::sort(mAnimationPass.begin(), mAnimationPass.end(),
		[](const AnimationInstance& a, const AnimationInstance& b) {
			return a.SkeletonID < b.SkeletonID;
		});


	for (uint32 i = 0; i < (uint32)mAnimationPass.size(); )
	{
		uint32 sk = mAnimationPass[i].SkeletonID;

		uint32 j = i + 1;
		while (j < mAnimationPass.size() &&
			mAnimationPass[j].SkeletonID == sk) {
			mAnimationPass[j].ReulstIndex = mAnimationPass[i].ReulstIndex + (j - i) * mAnimationPass[i].BoneCount;
			++j;
		}

		mAnimationBuckets.push_back({ i, j - i, mAnimationPass[i].BoneCount });
		i = j;
	}
	RENDERMANAGER.GetGroupBuffer(RENDERMANAGER.GetFrameResourceIndex())->AnimInstanceInfo->PushGraphicsData(mAnimationPass.data(), static_cast<uint32>(sizeof(AnimationInstance) * mAnimationPass.size()));

}

void AnimationSystem::AnimationCompute()
{

	AnimationDispatch();

}




void AnimationSystem::AnimationBlend(float deltaTime)
{


}

void AnimationSystem::AnimationDispatch()
{
	auto* res = RENDERMANAGER.GetGroupBuffer(FRAMERESOURCEIDNEX)
		->AnimResultInfo->GetBuffer().Get();

	auto b = CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	GRAPHICS_CMD_LIST->ResourceBarrier(1, &b);


	for (auto& b : mAnimationBuckets)
	{
		CSBatchCB cb{ b.start, b.count};
		GRAPHICS_CMD_LIST->SetComputeRoot32BitConstants(/*b0*/0, 2, &cb, 0);

		const uint32 groupsX = (b.bones + TX - 1) / TX;
		const uint32 groupsY = (b.count + TY - 1) / TY;

		GRAPHICS_CMD_LIST->Dispatch(groupsX, groupsY, 1);
	}


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


