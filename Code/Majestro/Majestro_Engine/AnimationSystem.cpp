#include "pch.h"
#include "AnimationSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Timer.h"
#include "Animator.h"
#include "AnimationComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"

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
			SkeletonBoneParams boneParam{};
			boneParam.matOffset = bone.matOffset;
			boneParam.blendWeight = bone.blendWeight;
			mBoneData.emplace_back(boneParam);
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
	RENDERMANAGER.GetAnimationBuffers()->SkeletonBone->PushDefaultToData(mBoneData.data(), static_cast<uint32>(mBoneData.size() * sizeof(SkeletonBoneParams)));

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
    auto view = mWorld->View<AnimationComponent>();
    for (Entity entity : view) {
        AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
        MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);
        MannequinComponent* mannequinComponent = mWorld->GetComponent<MannequinComponent>(entity);

        const uint32 previousClip = animCom->mAnimClipIdx;
        const uint32 previousUpperClip = animCom->mUpperAnimClipIdx;

        if (mainPlayerComponent) {
            animCom->mAnimClipIdx = mainPlayerComponent->mLowerStatePacket;
            animCom->mUpperAnimClipIdx = mainPlayerComponent->mStatePacket;

            // 수정: 참고 코드처럼 상하체가 다를 때만 Upper 레이어 활성화
            if (animCom->mAnimClipIdx != animCom->mUpperAnimClipIdx) {
                animCom->mEnableUpperBodyLayer = true;
                animCom->mUpperLayerWeight = 1.0f; // 수정: 명시적으로 1.0 설정
            }
            else {
                animCom->mEnableUpperBodyLayer = false;
                animCom->mUpperLayerWeight = 0.0f; // 수정: 비활성화 시 0.0
            }
        }

        if (mannequinComponent) {
            std::vector<Entity> choicdPlayerEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
            ChoicePlayerComponent* choicdPlayerComponent = mWorld->GetComponent<ChoicePlayerComponent>(choicdPlayerEntities[0]);
            if (mannequinComponent->mPlayerType == choicdPlayerComponent->mPlayerType)
                animCom->mAnimClipIdx = 1;
            else
                animCom->mAnimClipIdx = 0;
        }

        // 수정: 참고 코드처럼 Upper가 비활성화면 Lower와 동일하게 설정
        if (animCom->mEnableUpperBodyLayer == false) {
            animCom->mUpperAnimClipIdx = animCom->mAnimClipIdx;
            animCom->mUpperLayerWeight = 0.0f;
        }

        // 애니메이션 전환 감지 (Lower)
        if (animCom->mAnimClipIdx != previousClip) {
            animCom->mBlendClipIdx = previousClip;
            animCom->mBlendUpdateTime = animCom->mUpdateTime;
            animCom->mBlendTimer = 0.f;
            animCom->mBlendWeight = 1.f;
            animCom->mUpdateTime = 0.f;
        }

        // 애니메이션 전환 감지 (Upper)
        if (animCom->mEnableUpperBodyLayer && animCom->mUpperAnimClipIdx != previousUpperClip) {
            animCom->mUpperBlendClipIdx = previousUpperClip;
            animCom->mUpperBlendUpdateTime = animCom->mUpperUpdateTime;
            animCom->mUpperBlendTimer = 0.f;
            animCom->mUpperBlendWeight = 1.f;
            animCom->mUpperUpdateTime = 0.f;
        }

        // 타이머 업데이트
        animCom->mUpdateTime += deltaTime;
        animCom->mUpperUpdateTime += deltaTime;

        shared_ptr<Animator>& animClip = animCom->mAnimClips.at(animCom->mAnimClipIdx);
        shared_ptr<Animator>& upperAnimClip = animCom->mAnimClips.at(animCom->mUpperAnimClipIdx);

        // 루핑 처리
        if (animCom->mUpdateTime >= animClip->mDuration)
            animCom->mUpdateTime = 0.f;
        if (animCom->mUpperUpdateTime >= upperAnimClip->mDuration)
            animCom->mUpperUpdateTime = 0.f;

        // Lower 프레임 계산
        uint32 currentFrame = 0;
        uint32 nextFrame = 0;
        float ratio = 0.f;
        AnimationBlend(animClip, animCom->mUpdateTime, currentFrame, nextFrame, ratio);

        // Upper 프레임 계산
        uint32 upperCurrentFrame = 0;
        uint32 upperNextFrame = 0;
        float upperRatio = 0.f;
        AnimationBlend(upperAnimClip, animCom->mUpperUpdateTime, upperCurrentFrame, upperNextFrame, upperRatio);

        // Lower 블렌드 처리
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
        }

        // Upper 블렌드 처리
        uint32 upperBlendClipIdx = animCom->mUpperBlendClipIdx;
        uint32 upperBlendClipHandle = upperAnimClip->GetAnimClipHandle();
        uint32 upperBlendCurrentFrame = 0;
        uint32 upperBlendNextFrame = 0;
        float upperBlendRatio = 0.f;

        if (animCom->mEnableUpperBodyLayer &&
            animCom->mUpperBlendWeight > 0.f &&
            upperBlendClipIdx < animCom->mAnimClips.size())
        {
            animCom->mUpperBlendTimer += deltaTime;
            if (animCom->mUpperBlendDuration > 0.f)
                animCom->mUpperBlendWeight = max(0.f, 1.f - (animCom->mUpperBlendTimer / animCom->mUpperBlendDuration));
            else
                animCom->mUpperBlendWeight = 0.f;

            shared_ptr<Animator>& upperBlendClip = animCom->mAnimClips.at(upperBlendClipIdx);
            upperBlendClipHandle = upperBlendClip->GetAnimClipHandle();
            animCom->mUpperBlendUpdateTime += deltaTime;
            if (animCom->mUpperBlendUpdateTime >= upperBlendClip->mDuration)
                animCom->mUpperBlendUpdateTime = 0.f;

            AnimationBlend(upperBlendClip, animCom->mUpperBlendUpdateTime,
                upperBlendCurrentFrame, upperBlendNextFrame, upperBlendRatio);
        }
        else {
            animCom->mUpperBlendWeight = 0.f;
            animCom->mUpperBlendTimer = 0.f;
        }

        // AnimationInstance 구성
        AnimationInstance instance{};
        instance.SkeletonID = animClip->GetSkeleton()->GetSkeletonHandle();
        instance.AnimClipID = animClip->GetAnimClipHandle();
        instance.CurrentFrame = currentFrame;
        instance.NextFrame = nextFrame;
        instance.Ratio = ratio;
        instance.BoneCount = animCom->mAnimInstance.BoneCount;
        instance.ReulstIndex = 0;
        instance.EntityID = entity.GetID();

        // Lower 블렌드 정보
        instance.BlendClipID = blendClipHandle;
        instance.BlendCurrentFrame = blendCurrentFrame;
        instance.BlendNextFrame = blendNextFrame;
        instance.BlendRatio = blendRatio;
        instance.BlendWeight = animCom->mBlendWeight;
        instance.BlendMaskStart = animCom->mBlendMaskStart;
        instance.BlendMaskEnd = animCom->mBlendMaskEnd;
        instance.BlendMode = static_cast<uint32>(animCom->mBlendMode);

        // Upper 애니메이션 정보
        instance.UpperAnimClipIdx = upperAnimClip->GetAnimClipHandle();
        instance.UpperCurrentFrame = upperCurrentFrame;
        instance.UpperNextFrame = upperNextFrame;
        instance.UpperRatio = upperRatio;

        // Upper 블렌드 정보
        instance.UpperBlendClipIdx = upperBlendClipHandle;
        instance.UpperBlendCurrentFrame = upperBlendCurrentFrame;
        instance.UpperBlendNextFrame = upperBlendNextFrame;
        instance.UpperBlendRatio = upperBlendRatio;
        instance.UpperBlendWeight = animCom->mEnableUpperBodyLayer ? animCom->mUpperBlendWeight : 0.f;

        // 수정: 참고 코드처럼 UpperLayerWeight를 명확히 0 또는 1로 설정
        instance.UpperLayerWeight = animCom->mEnableUpperBodyLayer ? animCom->mUpperLayerWeight : 0.f;

        // 수정: UpperMask 범위 설정 (스켈레톤 구조에 맞게 설정 필요)
        // 예: 척추(Spine) 시작 본 인덱스 ~ 머리(Head) 끝 본 인덱스
        instance.UpperMaskStart = animCom->mUpperBlendMaskStart;
        instance.UpperMaskEnd = animCom->mUpperBlendMaskEnd;
        instance.UpperBlendMode = static_cast<uint32>(animCom->mUpperBlendMode);

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