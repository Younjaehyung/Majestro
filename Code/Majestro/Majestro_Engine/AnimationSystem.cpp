#include "pch.h"
#include "AnimationSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Timer.h"
#include "Animator.h"
#include "AnimationComponent.h"
#include "PlayerComponent.h"

#include "InputManager.h"

AnimationSystem::AnimationSystem(World* world) : System::System(world)
{

}

void AnimationSystem::Initialize() 
{
	// 불변 데이터  update


	uint32 skelHandle{};
	uint32 skelOffset{};
	for (auto& skels : RESOURCEMANAGER.GetAllResources<Skeleton>()) {
		shared_ptr<Skeleton> skel = dynamic_pointer_cast<Skeleton>(skels.second);

		skel->mStartOffset = skelOffset;
		for (auto& bone : skel->GetBones()) {
			mBoneData.emplace_back(bone.matOffset);
		}
		skelOffset += skel->GetBones().size();
		skel->SetSkeletonHandle(skelHandle++);
	}

	uint32 animClipHandle{};
	uint32 animClipOffset{};
	for (auto& animClip : RESOURCEMANAGER.GetAllResources<Animator>()) {
		shared_ptr<Animator> aniClip = dynamic_pointer_cast<Animator>(animClip.second);
		aniClip->SetAnimClipOffset(animClipOffset);
		aniClip->SetAnimClipHandle(animClipHandle++);
		aniClip->SetAnimSkelOffset(aniClip->GetSkeleton()->mStartOffset);
		for (const auto& v : aniClip->GetKeyFrames()) {
			
			mAniKeyFrame.insert(mAniKeyFrame.end(), v.begin(), v.end());
			animClipOffset += v.size();
		}
		mAniClipMeta.push_back(aniClip->GetClipMeta());
		
	}
	

	mAnimationBuckets.reserve(16);
	mAnimationShader = RESOURCEMANAGER.Get<Shader>(L"AnimationComputeShader");
	RENDERMANAGER.GetAnimationBuffers()->AnimationMeta ->PushDefaultToData(mAniClipMeta.data(), static_cast<uint32>(mAniClipMeta.size() * sizeof(AnimationClipMeta)));
	RENDERMANAGER.GetAnimationBuffers()->AnimationClip ->PushDefaultToData(mAniKeyFrame.data(), static_cast<uint32>(mAniKeyFrame.size() * sizeof(KeyFrameInfo)));
	RENDERMANAGER.GetAnimationBuffers()->SkeletonBone  ->PushDefaultToData(mBoneData.data(), static_cast<uint32>(mBoneData.size() * sizeof(Matrix)));
}


void AnimationSystem::Update(float deltaTime)
{
	if (false == mWorld->HasComponentPool<AnimationComponent>()) return;
	RENDERMANAGER.SetComputTable();

	ClearVector();
	AnimationPush(deltaTime);
	AnimationCompute();

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
	//auto computeFrames = [](const shared_ptr<Animator>& animClip, float updateTime, uint32& currentFrame, uint32& nextFrame, float& ratio) {
	//	const float frameRate = static_cast<float>(animClip->GetClipMeta().NumFrame / animClip->mDuration);
	//	currentFrame = static_cast<uint32>(updateTime * frameRate);
	//	currentFrame = min(currentFrame, animClip->mClipMeta.NumFrame - 1);
	//	nextFrame = currentFrame + 1 >= animClip->mClipMeta.NumFrame ? 0 : currentFrame + 1;

	//	const float frameTime = currentFrame / frameRate;
	//	const float nextFrameTime = nextFrame / frameRate;
	//	ratio = (updateTime - frameTime) / (nextFrameTime - frameTime);
	//	};


	auto view = mWorld->View<AnimationComponent>();
	for (Entity entity : view) {
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);

		const uint32 previousClip = animCom->mAnimClipIdx;
		if (mainPlayerComponent)
			animCom->mAnimClipIdx = mainPlayerComponent->mStatePacket;

		if (animCom->mAnimClipIdx != previousClip) {
			animCom->mBlendClipIdx = previousClip;
			animCom->mBlendUpdateTime = animCom->mUpdateTime;
			animCom->mBlendTimer = 0.f;
			animCom->mBlendWeight = 1.f;
			animCom->mUpdateTime = 0.f;
		}

		animCom->mUpdateTime += deltaTime;
		shared_ptr<Animator>& animClip = animCom->mAnimClips.at(animCom->mAnimClipIdx);

		if (animCom->mUpdateTime >= animClip->mDuration)
			animCom->mUpdateTime = 0.f;
		
		uint32 currentFrame = 0;
		uint32 nextFrame = 0;
		float ratio = 0.f;
		AnimationBlend(animClip, animCom->mUpdateTime, currentFrame, nextFrame, ratio);

		uint32 blendClipIdx = animCom->mBlendClipIdx;
		uint32 blendClipHandle = animClip->GetAnimClipHandle();
		uint32 blendCurrentFrame = 0;
		uint32 blendNextFrame = 0;
		float blendRatio = 0.f;

		if (animCom->mBlendWeight > 0.f && blendClipIdx < animCom->mAnimClips.size()) {
			animCom->mBlendTimer += deltaTime;
			if (animCom->mBlendDuration > 0.f)
				animCom->mBlendWeight = max(0.f, 1.f - (animCom->mBlendTimer / animCom->mBlendDuration));
			else
				animCom->mBlendWeight = 0.f;

			shared_ptr<Animator>& blendClip = animCom->mAnimClips.at(blendClipIdx);
			blendClipHandle = blendClip->GetAnimClipHandle();
			animCom->mBlendUpdateTime += deltaTime;
			if (animCom->mBlendUpdateTime >= blendClip->mDuration)
				animCom->mBlendUpdateTime = 0.f;

			AnimationBlend(blendClip, animCom->mBlendUpdateTime, blendCurrentFrame, blendNextFrame, blendRatio);
		}
		else {
			animCom->mBlendWeight = 0.f;
			animCom->mBlendTimer = 0.f;
			// blendClipIdx = animCom->mAnimClipIdx;
		}

		AnimationInstance instance{};
		instance.SkeletonID = animClip->GetSkeleton()->GetSkeletonHandle();
		instance.AnimClipID = animClip->GetAnimClipHandle();
		instance.CurrentFrame = currentFrame;
		instance.NextFrame = nextFrame;
		instance.Ratio = ratio;
		instance.BoneCount = animCom->mAnimInstance.BoneCount;
		instance.ReulstIndex = 0;
		instance.EntityID = entity.GetID();
		instance.BlendClipID = blendClipHandle;
		instance.BlendCurrentFrame = blendCurrentFrame;
		instance.BlendNextFrame = blendNextFrame;
		instance.BlendRatio = blendRatio;
		instance.BlendWeight = animCom->mBlendWeight;
		instance.BlendMaskStart = 2;//animCom->mBlendMaskStart;
		instance.BlendMaskEnd = 55; // animCom->mBlendMaskEnd;
		mAnimationPass.emplace_back(instance);
	}
	

	// 배치처리를 위한 정렬
	// 스켈레톤 ID 기준 오름차순 정렬
	std::sort(mAnimationPass.begin(), mAnimationPass.end(),
		[](const AnimationInstance& a, const AnimationInstance& b) {
			return a.SkeletonID < b.SkeletonID;
		});

	// 정렬 후 역매핑: 각 컴포넌트에 자기 위치 기록
	for (size_t i = 0; i < mAnimationPass.size(); ++i) {
		const Entity& e = mAnimationPass[i].EntityID;
		if (AnimationComponent* c = mWorld->GetComponent<AnimationComponent>(e)) {
			c->mAnimInstanceID = static_cast<uint32_t>(i); // 필요시 필드 추가
		}
	}

	uint32 resultIndex = 0;
	for (uint32_t i = 0; i < (uint32_t)mAnimationPass.size(); )
	{
		const uint32_t sk = mAnimationPass[i].SkeletonID;
		const uint32_t bones = mAnimationPass[i].BoneCount;

		// 같은 스켈레톤으로 이루어진 버킷의 끝 j 찾기
		uint32_t j = i + 1;
		while (j < (uint32_t)mAnimationPass.size() &&
			mAnimationPass[j].SkeletonID == sk)
		{
			// 같은 스켈레톤인데 BoneCount가 다르면 설계 오류
			assert(mAnimationPass[j].BoneCount == bones);
			++j;
		}

		// 이 버킷의 연속 팔레트 영역의 베이스
		const uint32_t base = resultIndex;

		// 버킷 내 모든 인스턴스의 시작 오프셋을 명시적으로 설정
		for (uint32_t k = i; k < j; ++k)
		{
			const uint32_t offsetInBucket = (k - i) * bones;
			mAnimationPass[k].ReulstIndex = base + offsetInBucket; // 단위: 행렬 인덱스
		}

		// 버킷 메타 저장(원하면 base/총행렬수도 함께 저장)
		mAnimationBuckets.push_back({ i, j - i, bones /* , base */ });

		// 다음 버킷 시작 위치로 커서 이동
		resultIndex += (j - i) * bones;

		i = j;
	}
	RENDERMANAGER.GetGroupBuffer(RENDERMANAGER.GetFrameResourceIndex())->AnimInstanceInfo->PushGraphicsData(mAnimationPass.data(), static_cast<uint32>(sizeof(AnimationInstance) * mAnimationPass.size()));

}

void AnimationSystem::AnimationCompute()
{
	COMPUTE_CMD_LIST->SetPipelineState(mAnimationShader->GetPipelineState().Get());

	auto* res = RENDERMANAGER.GetGroupBuffer(FRAMERESOURCEIDNEX)
		->AnimResultInfo->GetBuffer().Get();
	auto b = CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	COMPUTE_CMD_LIST->ResourceBarrier(1, &b);



	AnimationDispatch();



	{
		auto uav = CD3DX12_RESOURCE_BARRIER::UAV(res);
		COMPUTE_CMD_LIST->ResourceBarrier(1, &uav);
	}
	auto srv = CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON);
	COMPUTE_CMD_LIST->ResourceBarrier(1, &srv);

	const uint64 fenceValue = RENDERMANAGER.GetComputeCmdQueue()->ExecuteCommandList(RENDERMANAGER.GetFrameResourceIndex());
	RENDERMANAGER.SetAnimationComputeFenceValue(fenceValue);
}


void AnimationSystem::AnimationDispatch()
{
	for (Bucket& b : mAnimationBuckets)
	{
		CSBatchCB cb{ b.start, b.count};
		COMPUTE_CMD_LIST->SetComputeRoot32BitConstants(/*b0*/0, 2, &cb, 0);

		const uint32 groupsX = (b.bones + TX - 1) / TX;
		const uint32 groupsY = (b.count + TY - 1) / TY;

		COMPUTE_CMD_LIST->Dispatch(groupsX, groupsY, 1);
	}
}

void AnimationSystem::AnimationBlend(const shared_ptr<Animator>& animClip, float updateTime, uint32& currentFrame, uint32& nextFrame, float& ratio)
{
	const float frameRate = static_cast<float>(animClip->GetClipMeta().NumFrame / animClip->mDuration);
	currentFrame = static_cast<uint32>(updateTime * frameRate);
	currentFrame = min(currentFrame, animClip->mClipMeta.NumFrame - 1);
	nextFrame = currentFrame + 1 >= animClip->mClipMeta.NumFrame ? 0 : currentFrame + 1;

	const float frameTime = currentFrame / frameRate;
	const float nextFrameTime = nextFrame / frameRate;
	ratio = (updateTime - frameTime) / (nextFrameTime - frameTime);

}