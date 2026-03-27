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
            boneParam.parentIdx = bone.parentIdx;
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
    std::sort(mAniClipMeta.begin(), mAniClipMeta.end(), [](const AnimationClipMeta& a, const AnimationClipMeta& b) {
        return a.AnimOffset < b.AnimOffset;
        });

	mAnimationBuckets.reserve(16);
	mAnimationShader = RESOURCEMANAGER.Get<Shader>(L"AnimationComputeShader");
	RENDERMANAGER.GetAnimationBuffers()->AnimationMeta ->PushDefaultToData(mAniClipMeta.data(), static_cast<uint32>(mAniClipMeta.size() * sizeof(AnimationClipMeta)));
	RENDERMANAGER.GetAnimationBuffers()->AnimationClip ->PushDefaultToData(mAniKeyFrame.data(), static_cast<uint32>(mAniKeyFrame.size() * sizeof(KeyFrameInfo)));
	RENDERMANAGER.GetAnimationBuffers()->SkeletonBone->PushDefaultToData(mBoneData.data(), static_cast<uint32>(mBoneData.size() * sizeof(SkeletonBoneParams)));

	// ── 엔티티별 BoneCount · UpperMask 자동 설정 ──────────────────────────
	// [버그 수정] AnimationInstance.BoneCount가 0인 채로 남아 팔레트 오프셋이
	// 모두 0이 되는 문제 수정. Skeleton에서 실제 본 개수를 읽어 초기화한다.
	// [버그 수정] mUpperBlendMaskStart/End 하드코딩(2/66) 제거.
	// FBXData.cpp 로딩 시 계산된 Skeleton의 Spine 인덱스로 설정한다.
	if (mWorld->HasComponentPool<AnimationComponent>())
	{
		auto view = mWorld->View<AnimationComponent>();
		for (Entity entity : view)
		{
			AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
			if (!animCom || animCom->mAnimClips.empty()) continue;

			auto& skel = animCom->mAnimClips[0]->GetSkeleton();
			if (!skel) continue;

			animCom->mAnimInstance.BoneCount = skel->mBoneCount;
			animCom->mUpperBlendMaskStart    = skel->mSpineBoneStartOffset;
			animCom->mUpperBlendMaskEnd      = skel->mSpineBoneEndOffset;
		}
	}
	// ────────────────────────────────────────────────────────────────────────

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

        if (animCom->mAnimClips.empty()) continue;
        const uint32 clipCount = static_cast<uint32>(animCom->mAnimClips.size());

        const uint32 previousLowerClip  = animCom->mLowerAnimClipIdx;
        const uint32 previousUpperClip  = animCom->mUpperAnimClipIdx;
        const bool   wasUpperEnabled    = animCom->mEnableUpperBodyLayer;


        // ── 1. 서버 패킷 / 외부 입력으로부터 클립 인덱스 수신 ─────────────
        if (mainPlayerComponent)
        {
            const uint32 newLower = min(mainPlayerComponent->mLowerStatePacket, clipCount - 1);
            const uint32 newUpper = min(mainPlayerComponent->mStatePacket,      clipCount - 1);
            animCom->mLowerAnimClipIdx = newLower;
            animCom->mUpperAnimClipIdx = newUpper;

        // std::cout << "Lower Clip: " << mainPlayerComponent->mLowerStatePacket << ", Upper Clip: " << mainPlayerComponent->mStatePacket << std::endl;
        }

		

        if (mannequinComponent)
        {
            std::vector<Entity> choicdPlayerEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
            ChoicePlayerComponent* choicdPlayerComponent = mWorld->GetComponent<ChoicePlayerComponent>(choicdPlayerEntities[0]);
            animCom->mLowerAnimClipIdx = (mannequinComponent->mPlayerType == choicdPlayerComponent->mPlayerType) ? 1u : 0u;
        }

        // ── 2. Upper 레이어 활성/비활성 목표 결정 ────────────────────────
        // [수정] 상체/하체가 다르면 Upper 레이어를 켠다.
        const bool upperNeeded = (animCom->mLowerAnimClipIdx != animCom->mUpperAnimClipIdx);

        if (upperNeeded)
        {
            animCom->mEnableUpperBodyLayer = true;
            animCom->mUpperLayerTargetWeight = 1.0f;
        }
        else
        {
            animCom->mUpperLayerTargetWeight = 0.0f;
        }

        // [수정] UpperLayerWeight를 목표값으로 보간
        {
            const float targetW = animCom->mUpperLayerTargetWeight;
            const float currentW = animCom->mUpperLayerWeight;

            if (currentW < targetW)
            {
                const float step = (animCom->mUpperLayerEnterDuration > 0.f)
                    ? deltaTime / animCom->mUpperLayerEnterDuration : 1.f;
                animCom->mUpperLayerWeight = min(targetW, currentW + step);
            }
            else if (currentW > targetW)
            {
                const float step = (animCom->mUpperLayerExitDuration > 0.f)
                    ? deltaTime / animCom->mUpperLayerExitDuration : 1.f;
                animCom->mUpperLayerWeight = max(targetW, currentW - step);
            }

            if (animCom->mUpperLayerWeight <= 0.f && targetW <= 0.f)
                animCom->mEnableUpperBodyLayer = false;
        }

        // ── 3. Lower 클립 전환 감지 → 블렌딩 시작 ──────────────────────
        if (animCom->mLowerAnimClipIdx != previousLowerClip)
        {
            animCom->mLowerBlendClipIdx    = previousLowerClip;
            animCom->mLowerBlendUpdateTime = animCom->mLowerUpdateTime;
            animCom->mLowerBlendTimer      = 0.f;
            animCom->mLowerBlendWeight     = 1.f;
            animCom->mLowerUpdateTime      = 0.f;

            if (animCom->mAnimationGraph)
            {
                animCom->mLowerBlendDuration = animCom->mAnimationGraph->GetLowerDuration(
                    previousLowerClip, animCom->mLowerAnimClipIdx);
                animCom->mLowerBlendCurve = animCom->mAnimationGraph->GetLowerCurve(
                    previousLowerClip, animCom->mLowerAnimClipIdx);
            }
        }

        // ── 4. Upper 클립 전환 감지 → 블렌딩 시작 ──────────────────────
        // 최초 활성화(wasUpperEnabled == false)일 때는 블렌딩 없이 즉시 재생.
        // 이미 활성화된 상태에서 클립이 바뀔 때만 블렌딩을 시작한다.
        if (!wasUpperEnabled && animCom->mEnableUpperBodyLayer)
        {
            // 최초 활성화: 이전 클립 흔적 없이 Upper 즉시 시작
            animCom->mUpperBlendWeight = 0.f;
            animCom->mUpperBlendTimer  = 0.f;
            animCom->mUpperUpdateTime  = 0.f;
        }
        else if (wasUpperEnabled && animCom->mUpperAnimClipIdx != previousUpperClip)
        {
            // 이미 활성화 상태에서 클립 변경 → 이전 포즈에서 부드럽게 전환
            animCom->mUpperBlendClipIdx    = previousUpperClip;
            animCom->mUpperBlendUpdateTime = animCom->mUpperUpdateTime;
            animCom->mUpperBlendTimer      = 0.f;
            animCom->mUpperBlendWeight     = 1.f;
            animCom->mUpperUpdateTime      = 0.f;

            if (animCom->mAnimationGraph)
            {
                animCom->mUpperBlendDuration = animCom->mAnimationGraph->GetUpperDuration(
                    previousUpperClip, animCom->mUpperAnimClipIdx);
                animCom->mUpperBlendCurve = animCom->mAnimationGraph->GetUpperCurve(
                    previousUpperClip, animCom->mUpperAnimClipIdx);
            }
        }

        // ── 5. 재생 시간 전진 ─────────────────────────────────────────
        animCom->mLowerUpdateTime += deltaTime;
        animCom->mUpperUpdateTime += deltaTime;

        shared_ptr<Animator>& lowerAnimClip      = animCom->mAnimClips.at(animCom->mLowerAnimClipIdx);
        shared_ptr<Animator>& upperAnimClip = animCom->mAnimClips.at(animCom->mUpperAnimClipIdx);

        // [버그 수정] 루핑: 초과분을 버리지 않고 fmod로 정확하게 처리
        const float lowerDur = max(static_cast<float>(lowerAnimClip->mDuration),      0.0001f);
        const float upperDur = max(static_cast<float>(upperAnimClip->mDuration), 0.0001f);
        animCom->mLowerUpdateTime      = fmodf(animCom->mLowerUpdateTime,      lowerDur);
        animCom->mUpperUpdateTime = fmodf(animCom->mUpperUpdateTime, upperDur);

        // ── 6. 프레임 계산 ───────────────────────────────────────────
        uint32 currentFrame = 0, nextFrame = 0;       float ratio = 0.f;
        uint32 upperCurrentFrame = 0, upperNextFrame = 0; float upperRatio = 0.f;
        AnimationBlend(lowerAnimClip,      animCom->mLowerUpdateTime,      currentFrame,      nextFrame,      ratio);
        AnimationBlend(upperAnimClip, animCom->mUpperUpdateTime, upperCurrentFrame, upperNextFrame, upperRatio);

        // ── 7. Lower 블렌딩 처리 ─────────────────────────────────────
        uint32 blendClipIdx = animCom->mLowerBlendClipIdx;
        uint32 blendClipHandle = lowerAnimClip->GetAnimClipHandle();
        uint32 blendCurrentFrame = 0, blendNextFrame = 0; float blendRatio = 0.f;

        if (animCom->mLowerBlendWeight > 0.f && blendClipIdx < clipCount)
        {
            animCom->mLowerBlendTimer += deltaTime;
            if (animCom->mLowerBlendDuration > 0.f)
            {
                const float rawT = animCom->mLowerBlendTimer / animCom->mLowerBlendDuration;
                animCom->mLowerBlendWeight = max(0.f, 1.f - ApplyCurve(rawT, animCom->mLowerBlendCurve));
            }
            else
                animCom->mLowerBlendWeight = 0.f;

            shared_ptr<Animator>& blendClip = animCom->mAnimClips.at(blendClipIdx);
            blendClipHandle = blendClip->GetAnimClipHandle();
            animCom->mLowerBlendUpdateTime += deltaTime;
            animCom->mLowerBlendUpdateTime = fmodf(animCom->mLowerBlendUpdateTime,
                max(static_cast<float>(blendClip->mDuration), 0.0001f));
            AnimationBlend(blendClip, animCom->mLowerBlendUpdateTime, blendCurrentFrame, blendNextFrame, blendRatio);
        }
        else
        {
            animCom->mLowerBlendWeight = 0.f;
            animCom->mLowerBlendTimer  = 0.f;
        }

        // ── 8. Upper 블렌딩 처리 ─────────────────────────────────────
        uint32 upperBlendClipIdx = animCom->mUpperBlendClipIdx;
        uint32 upperBlendClipHandle = upperAnimClip->GetAnimClipHandle();
        uint32 upperBlendCurrentFrame = 0, upperBlendNextFrame = 0; float upperBlendRatio = 0.f;

        if (animCom->mUpperBlendWeight > 0.f && upperBlendClipIdx < clipCount)
        {
            animCom->mUpperBlendTimer += deltaTime;
            if (animCom->mUpperBlendDuration > 0.f)
            {
                const float rawT = animCom->mUpperBlendTimer / animCom->mUpperBlendDuration;
                animCom->mUpperBlendWeight = max(0.f, 1.f - ApplyCurve(rawT, animCom->mUpperBlendCurve));
            }
            else
                animCom->mUpperBlendWeight = 0.f;

            shared_ptr<Animator>& upperBlendClip = animCom->mAnimClips.at(upperBlendClipIdx);
            upperBlendClipHandle = upperBlendClip->GetAnimClipHandle();
            animCom->mUpperBlendUpdateTime += deltaTime;
            animCom->mUpperBlendUpdateTime = fmodf(animCom->mUpperBlendUpdateTime,
                max(static_cast<float>(upperBlendClip->mDuration), 0.0001f));
            AnimationBlend(upperBlendClip, animCom->mUpperBlendUpdateTime,
                upperBlendCurrentFrame, upperBlendNextFrame, upperBlendRatio);
        }
        else
        {
            animCom->mUpperBlendWeight = 0.f;
            animCom->mUpperBlendTimer  = 0.f;
        }

        // ── 9. Additive 레퍼런스 포즈 계산 ──────────────────────────
        // mUpperRefClipIdx가 설정된 경우 해당 클립의 첫 프레임을 레퍼런스로 사용.
        // UINT32_MAX(기본값)이면 RefClipID를 sentinel(0xFFFFFFFF)로 설정해
        // HLSL에서 Override 모드로 폴백한다.
        uint32 refClipHandle    = 0xFFFFFFFFu;
        uint32 refCurrentFrame  = 0, refNextFrame = 0; float refRatio = 0.f;

        const uint32 refClipIdx = animCom->mUpperRefClipIdx;
        if (refClipIdx != UINT32_MAX && refClipIdx < clipCount)
        {
            shared_ptr<Animator>& refClip = animCom->mAnimClips.at(refClipIdx);
            refClipHandle = refClip->GetAnimClipHandle();
            // 레퍼런스는 첫 프레임 고정 (중립 정지 포즈)
            AnimationBlend(refClip, 0.f, refCurrentFrame, refNextFrame, refRatio);
        }

        // ── 10. AnimationInstance 구성 ───────────────────────────────
        const uint32 boneCount = animCom->mAnimInstance.BoneCount;

        AnimationInstance instance{};
        instance.EntityID = entity.GetID();
        instance.SkeletonID    = lowerAnimClip->GetSkeleton()->GetSkeletonHandle();
        instance.BoneCount = boneCount;
        instance.ReulstIndex = 0;
        instance.UpperLayerWeight = animCom->mUpperLayerWeight;  // 점진적 보간 값

		// Lower 클립
        instance.LowerAnimClipID    = lowerAnimClip->GetAnimClipHandle();
        instance.LowerCurrentFrame  = currentFrame;
        instance.LowerNextFrame     = nextFrame;
        instance.LowerRatio         = ratio;
        
        
        

        // Lower 블렌드
        instance.LowerBlendClipIdx       = blendClipHandle;
        instance.LowerBlendCurrentFrame = blendCurrentFrame;
        instance.LowerBlendNextFrame    = blendNextFrame;
        instance.LowerBlendRatio        = blendRatio;
        instance.LowerBlendWeight       = animCom->mLowerBlendWeight;


        instance.BlendMaskStart    = 0;
        instance.BlendMaskEnd      = boneCount;
        instance.BlendMode         = static_cast<uint32>(animCom->mLowerBlendMode);







        // Upper 클립
        instance.UpperAnimClipID  = upperAnimClip->GetAnimClipHandle();
        instance.UpperCurrentFrame = upperCurrentFrame;
        instance.UpperNextFrame    = upperNextFrame;
        instance.UpperRatio        = upperRatio;

        // Upper 블렌드
        instance.UpperBlendClipIdx       = upperBlendClipHandle;
        instance.UpperBlendCurrentFrame  = upperBlendCurrentFrame;
        instance.UpperBlendNextFrame     = upperBlendNextFrame;
        instance.UpperBlendRatio         = upperBlendRatio;
        instance.UpperBlendWeight        = animCom->mUpperBlendWeight;
        


        instance.UpperMaskStart = min(animCom->mUpperBlendMaskStart, boneCount - 1);
        instance.UpperMaskEnd   = min(animCom->mUpperBlendMaskEnd, boneCount - 1);
        instance.UpperBlendMode = static_cast<uint32>(animCom->mUpperBlendMode);

        // Additive 레퍼런스 포즈
        instance.RefClipID       = refClipHandle;
        instance.RefCurrentFrame = refCurrentFrame;
        instance.RefNextFrame    = refNextFrame;
        instance.RefRatio        = refRatio;

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

        const uint32 groupsX = 1;
		const uint32 groupsY = (b.count + TY - 1) / TY;

		COMPUTE_CMD_LIST->Dispatch(groupsX, groupsY, 1);
	}
}

void AnimationSystem::AnimationBlend(const shared_ptr<Animator>& animClip, float updateTime, uint32& currentFrame, uint32& nextFrame, float& ratio)
{
    const uint32 numFrame = max(animClip->mClipMeta.NumFrame, 1u);
    const float duration = max(static_cast<float>(animClip->mDuration), 0.0001f);
    const float frameDuration = duration / static_cast<float>(numFrame);

    float localTime = fmodf(max(updateTime, 0.f), duration);
    if (localTime < 0.f)
        localTime += duration;

    currentFrame = min(static_cast<uint32>(localTime / frameDuration), numFrame - 1);
    nextFrame = (currentFrame + 1) % numFrame;

    const float currentFrameTime = static_cast<float>(currentFrame) * frameDuration;
    ratio = (localTime - currentFrameTime) / frameDuration;
    ratio = std::clamp(ratio, 0.f, 1.f);
}

float AnimationSystem::ApplyCurve(float t, AnimBlendCurve curve)
{
    t = std::clamp(t, 0.f, 1.f);
    switch (curve)
    {
    case AnimBlendCurve::EaseIn:
        // 천천히 시작 → 빠르게 끝 (2차 곡선)
        return t * t;
    case AnimBlendCurve::EaseOut:
        // 빠르게 시작 → 천천히 끝 (반전 2차 곡선)
        return 1.f - (1.f - t) * (1.f - t);
    case AnimBlendCurve::EaseInOut:
        // 시작·끝 모두 부드럽게
        return t < 0.5f
            ? 2.f * t * t
            : 1.f - 2.f * (1.f - t) * (1.f - t);
    default:
        return t; // Linear
    }
}