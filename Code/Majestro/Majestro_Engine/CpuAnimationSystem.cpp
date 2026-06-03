#include "pch.h"
#include "CpuAnimationSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Timer.h"
#include "Animator.h"
#include "AnimationComponent.h"
#include "AnimationEvaluator.h"
#include "PlayerComponent.h"
#include "PlayerAnimationResolver.h"
#include "EnemyComponent.h"
#include "TagComponent.h"
#include "RenderComponent.h"
#include "MovementComponent.h"
#include "MovementSystem.h"
#include "PlayerSystem.h"
#include "DamageFeedbackComponent.h"

CpuAnimationSystem::CpuAnimationSystem(World* world) : System::System(world)
{
}

std::vector<std::type_index> CpuAnimationSystem::After() const
{
	return { typeid(MovementSystem), typeid(PlayerSystem) };
}

void CpuAnimationSystem::Initialize()
{	// 기존 Animation과 동일
	uint32 skelHandle{};
	uint32 skelOffset{};
	for (auto& skels : RESOURCEMANAGER.GetAllResources<Skeleton>()) {
		shared_ptr<Skeleton> skel = dynamic_pointer_cast<Skeleton>(skels.second);

		
		skel->BuildAimBoneIndices();
		skel->BuildUpperBodyMaskRange();
		skel->BuildBoneNameIndex();

		skel->mStartOffset = skelOffset;
		for (auto& bone : skel->GetBones()) {
			SkeletonBoneParams boneParam{};
			boneParam.matOffset = bone.matOffset;
			boneParam.blendWeight = bone.blendWeight;
			boneParam.parentIdx = bone.parentIdx;
			mBoneData.emplace_back(boneParam);
		}
		skelOffset += static_cast<uint32>(skel->GetBones().size());
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
			animClipOffset += static_cast<uint32>(v.size());
		}
		mAniClipMeta.push_back(aniClip->GetClipMeta());
	}
	std::sort(mAniClipMeta.begin(), mAniClipMeta.end(),
		[](const AnimationClipMeta& a, const AnimationClipMeta& b) {
			return a.AnimOffset < b.AnimOffset;
		});

	mAnimationPass.reserve(64);
	mFinalBoneUpload.reserve(64 * 100);
	mScratchModelBones.reserve(256);

	RENDERMANAGER.GetAnimationBuffers()->AnimationMeta->PushDefaultToData(
		mAniClipMeta.data(), static_cast<uint32>(mAniClipMeta.size() * sizeof(AnimationClipMeta)));
	RENDERMANAGER.GetAnimationBuffers()->AnimationClip->PushDefaultToData(
		mAniKeyFrame.data(), static_cast<uint32>(mAniKeyFrame.size() * sizeof(KeyFrameInfo)));
	RENDERMANAGER.GetAnimationBuffers()->SkeletonBone->PushDefaultToData(
		mBoneData.data(), static_cast<uint32>(mBoneData.size() * sizeof(SkeletonBoneParams)));
}

void CpuAnimationSystem::Update(float deltaTime)
{
	if (false == mWorld->HasComponentPool<AnimationComponent>()) return;

	ClearVector();
	AnimationPush(deltaTime);
	EvaluateAndUpload();
}

void CpuAnimationSystem::ClearVector()
{
	mAnimationPass.clear();
	mAimPass.clear();
	mFinalBoneUpload.clear();
	mEntityModelBones.clear();
}

void CpuAnimationSystem::AnimationPush(float deltaTime)
{
	auto view = mWorld->View<AnimationComponent>();
	for (Entity entity : view) {
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity);
		MainPlayerComponent* mainPlayerCom = mWorld->GetComponent<MainPlayerComponent>(entity);
		EnemyComponent* enemyCom = mWorld->GetComponent<EnemyComponent>(entity);
		MannequinComponent* mannequinCom = mWorld->GetComponent<MannequinComponent>(entity);
		TransformComponent* transformCom = mWorld->GetComponent<TransformComponent>(entity);
		PlayerMovementComponent* movementCom = mWorld->GetComponent<PlayerMovementComponent>(entity);
		NetTransformComponent* netTransformCom = mWorld->GetComponent<NetTransformComponent>(entity);

		const uint32 previousClip = animCom->mLowerAnimClipIdx;
		const uint32 previousUpperClip = animCom->mUpperAnimClipIdx;

		// 이번 프레임 resolver가 원하는 상체 레이어 활성 목표.
		bool desiredUpperLayer = false;

		if (mainPlayerCom) {
			const PlayerAnimationResolveResult resolvedAnim = ResolvePlayerAnimationState(
				*mainPlayerCom, *animCom, transformCom, movementCom, netTransformCom);

			animCom->mLowerAnimClipIdx = resolvedAnim.LowerClipIndex;
			desiredUpperLayer = resolvedAnim.EnableUpperLayer;

			// 레이어를 켜는 동안에만 상체 클립 갱신
			if (desiredUpperLayer)
				animCom->mUpperAnimClipIdx = resolvedAnim.UpperClipIndex;
		}

		if (enemyCom) {
			const int idx = enemyCom->mAnimState;
			if (idx >= 0 && idx < static_cast<int>(animCom->mAnimClips.size()))
				animCom->mLowerAnimClipIdx = idx;
		}

		if (mannequinCom) {
			std::vector<Entity> choicdPlayerEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
			ChoicePlayerComponent* choicdPlayerComponent = mWorld->GetComponent<ChoicePlayerComponent>(choicdPlayerEntities[0]);
			const bool selected = (mannequinCom->mPlayerType == choicdPlayerComponent->mPlayerType);
			animCom->mLowerAnimClipIdx = selected ? 1 : 0;

			// 선택된 캐릭터는 원래 머티리얼, 나머지는 타이틀 맵의 Solid 셰이더로 표시
			if (RenderComponent* renderCom = mWorld->GetComponent<RenderComponent>(entity)) {
				std::vector<shared_ptr<Material>>& target =
					selected ? mannequinCom->mNormalMaterials : mannequinCom->mSolidMaterials;
				if (!target.empty())
					renderCom->mMaterials = target;
			}
		}

		// 상체 레이어 가중치
		if (mainPlayerCom) {
			if (desiredUpperLayer) {	// 진입 : 반응성 위해 즉시 1.0 (상체 클립 크로스페이드가 진입 부드러움 담당)
				animCom->mUpperLayerWeight = 1.0f;
			}
			else {	// 종료 : mUpperLayerFadeOutDuration 동안 1->0 페이드아웃
				const float fadeDuration = max(animCom->mUpperLayerFadeOutDuration, 0.0001f);
				animCom->mUpperLayerWeight = max(0.f, animCom->mUpperLayerWeight - deltaTime / fadeDuration);
			}

			// 가중치가 남아있는 동안에는 레이어를 살려둬 상체 클립을 계속 샘플링
			animCom->mEnableUpperBodyLayer = (animCom->mUpperLayerWeight > 0.0001f);

			// 완전히 꺼졌을 때만 상체 클립을 하체와 동기화
			if (animCom->mEnableUpperBodyLayer == false) {
				animCom->mUpperAnimClipIdx = animCom->mLowerAnimClipIdx;
				animCom->mUpperLayerWeight = 0.0f;
			}
		}

		const bool forceLowerRestart = mainPlayerCom &&
			mainPlayerCom->mStateSequence != animCom->mConsumedPlayerStateSequence &&
			animCom->mLowerAnimClipIdx == previousClip &&
			(mainPlayerCom->mLowerState != mainPlayerCom->mPrevLowerStatePacket ||
				animCom->mEnableUpperBodyLayer == false);
		const bool forceUpperRestart = mainPlayerCom &&
			mainPlayerCom->mStateSequence != animCom->mConsumedPlayerStateSequence &&
			animCom->mEnableUpperBodyLayer &&
			animCom->mUpperAnimClipIdx == previousUpperClip;

		if (animCom->mLowerAnimClipIdx != previousClip || forceLowerRestart) {
			animCom->mBlendClipIdx = previousClip;
			animCom->mBlendUpdateTime = animCom->mUpdateTime;
			animCom->mBlendTimer = 0.f;
			animCom->mBlendWeight = 1.f;
			animCom->mUpdateTime = 0.f;
		}

		// 페이드아웃 중(desiredUpperLayer==false)에는 상체 클립을 유지해야 하므로 재시작/전환 막음
		if (desiredUpperLayer && animCom->mEnableUpperBodyLayer && (animCom->mUpperAnimClipIdx != previousUpperClip || forceUpperRestart)) {
			animCom->mUpperBlendClipIdx = previousUpperClip;
			animCom->mUpperBlendUpdateTime = animCom->mUpperUpdateTime;
			animCom->mUpperBlendTimer = 0.f;
			animCom->mUpperBlendWeight = 1.f;
			animCom->mUpperUpdateTime = 0.f;
		}

		if (mainPlayerCom)
			animCom->mConsumedPlayerStateSequence = mainPlayerCom->mStateSequence;

		animCom->mUpdateTime += deltaTime;
		animCom->mUpperUpdateTime += deltaTime;

		shared_ptr<Animator>& animClip = animCom->mAnimClips.at(animCom->mLowerAnimClipIdx);
		shared_ptr<Animator>& upperAnimClip = animCom->mAnimClips.at(animCom->mUpperAnimClipIdx);

		if (animCom->mUpdateTime >= animClip->mDuration)
			animCom->mUpdateTime = 0.f;
		if (animCom->mUpperUpdateTime >= upperAnimClip->mDuration)
			animCom->mUpperUpdateTime = 0.f;

		uint32 currentFrame = 0;
		uint32 nextFrame = 0;
		float ratio = 0.f;
		AnimationBlend(animClip, animCom->mUpdateTime, currentFrame, nextFrame, ratio);

		uint32 upperCurrentFrame = 0;
		uint32 upperNextFrame = 0;
		float upperRatio = 0.f;
		AnimationBlend(upperAnimClip, animCom->mUpperUpdateTime, upperCurrentFrame, upperNextFrame, upperRatio);

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
		instance.BlendMaskStart = animCom->mBlendMaskStart;
		instance.BlendMaskEnd = animCom->mBlendMaskEnd;
		instance.BlendMode = static_cast<uint32>(animCom->mBlendMode);

		instance.UpperAnimClipIdx = upperAnimClip->GetAnimClipHandle();
		instance.UpperCurrentFrame = upperCurrentFrame;
		instance.UpperNextFrame = upperNextFrame;
		instance.UpperRatio = upperRatio;

		instance.UpperBlendClipIdx = upperBlendClipHandle;
		instance.UpperBlendCurrentFrame = upperBlendCurrentFrame;
		instance.UpperBlendNextFrame = upperBlendNextFrame;
		instance.UpperBlendRatio = upperBlendRatio;
		instance.UpperBlendWeight = animCom->mEnableUpperBodyLayer ? animCom->mUpperBlendWeight : 0.f;
		instance.UpperLayerWeight = animCom->mEnableUpperBodyLayer ? animCom->mUpperLayerWeight : 0.f;

		instance.UpperMaskStart = min(animCom->mUpperBlendMaskStart, instance.BoneCount > 0 ? instance.BoneCount - 1 : 0);
		instance.UpperMaskEnd = min(animCom->mUpperBlendMaskEnd, instance.BoneCount > 0 ? instance.BoneCount - 1 : 0);
		if (instance.UpperMaskStart > instance.UpperMaskEnd)
			std::swap(instance.UpperMaskStart, instance.UpperMaskEnd);
		instance.UpperBlendMode = static_cast<uint32>(animCom->mUpperBlendMode);

		// AimOffset / Hit reaction 파라미터 세팅.
		AimParams aim{};
		auto* hitReaction = mWorld->GetComponent<HitReactionComponent>(entity);
		if (animCom->mEnableAimOffset || hitReaction)
		{
			if (animCom->mEnableAimOffset)
			{
				aim.AimPitch = animCom->mAimPitch;
				aim.AimYaw   = animCom->mAimYaw;
			}
			if (hitReaction)
			{
				aim.HitPitch = hitReaction->mCurrentPitch;
				aim.HitYaw   = hitReaction->mCurrentYaw;
			}

			if (const shared_ptr<Skeleton>& sk = animClip->GetSkeleton())
			{
				aim.Spine1Idx = sk->mSpine1Idx;
				aim.Spine2Idx = sk->mSpine2Idx;
				aim.Spine3Idx = sk->mSpine3Idx;
				aim.NeckIdx   = sk->mNeckIdx;

				aim.Spine1Weight = (aim.Spine1Idx != Skeleton::INVALID_BONE_INDEX) ? 0.30f : 0.f;
				aim.Spine2Weight = (aim.Spine2Idx != Skeleton::INVALID_BONE_INDEX) ? 0.30f : 0.f;
				aim.Spine3Weight = (aim.Spine3Idx != Skeleton::INVALID_BONE_INDEX) ? 0.25f : 0.f;
				aim.NeckWeight   = (aim.NeckIdx   != Skeleton::INVALID_BONE_INDEX) ? 0.15f : 0.f;

				const float weightSum = aim.Spine1Weight + aim.Spine2Weight + aim.Spine3Weight + aim.NeckWeight;
				if (weightSum > 0.0001f)
				{
					aim.Spine1Weight /= weightSum;
					aim.Spine2Weight /= weightSum;
					aim.Spine3Weight /= weightSum;
					aim.NeckWeight   /= weightSum;
				}
			}
		}

		mAnimationPass.emplace_back(instance);
		mAimPass.emplace_back(aim);
	}

	// mAnimationPass와 mAimPass를 동일 permutation으로 정렬
	{
		const size_t n = mAnimationPass.size();
		vector<uint32> order(n);
		for (uint32 i = 0; i < static_cast<uint32>(n); ++i) order[i] = i;
		std::sort(order.begin(), order.end(),
			[this](uint32 a, uint32 b) {
				return mAnimationPass[a].SkeletonID < mAnimationPass[b].SkeletonID;
			});

		vector<AnimationInstance> sortedInst(n);
		vector<AimParams>         sortedAim(n);
		for (size_t i = 0; i < n; ++i)
		{
			sortedInst[i] = mAnimationPass[order[i]];
			sortedAim[i]  = mAimPass[order[i]];
		}
		mAnimationPass.swap(sortedInst);
		mAimPass.swap(sortedAim);
	}

	for (size_t i = 0; i < mAnimationPass.size(); ++i) {
		const Entity& e = mAnimationPass[i].EntityID;
		if (AnimationComponent* c = mWorld->GetComponent<AnimationComponent>(e)) {
			c->mAnimInstanceID = static_cast<uint32_t>(i);
		}
	}

	uint32 resultIndex = 0;
	for (uint32_t i = 0; i < (uint32_t)mAnimationPass.size(); )
	{
		const uint32_t sk = mAnimationPass[i].SkeletonID;
		const uint32_t bones = mAnimationPass[i].BoneCount;

		uint32_t j = i + 1;
		while (j < (uint32_t)mAnimationPass.size() &&
			mAnimationPass[j].SkeletonID == sk)
		{
			assert(mAnimationPass[j].BoneCount == bones);
			++j;
		}

		const uint32_t base = resultIndex;
		for (uint32_t k = i; k < j; ++k)
		{
			const uint32_t offsetInBucket = (k - i) * bones;
			mAnimationPass[k].ReulstIndex = base + offsetInBucket;
		}
		resultIndex += (j - i) * bones;
		i = j;
	}

	// AnimInstance 버퍼는 MotionVectorPass 등 다른 셰이더가 참조하므로 그대로 업로드
	if (!mAnimationPass.empty())
	{
		RENDERMANAGER.GetGroupBuffer(RENDERMANAGER.GetFrameResourceIndex())->AnimInstanceInfo
			->PushGraphicsData(mAnimationPass.data(),
				static_cast<uint32>(sizeof(AnimationInstance) * mAnimationPass.size()));
	}
}

void CpuAnimationSystem::EvaluateAndUpload()
{
	if (mAnimationPass.empty())
		return;

	// 전체 본 행렬 개수 = 마지막 인스턴스 베이스 + 본 개수
	uint32 totalBones = 0;
	for (const AnimationInstance& inst : mAnimationPass)
	{
		totalBones = max(totalBones, inst.ReulstIndex + inst.BoneCount);
	}
	if (totalBones == 0)
		return;

	mFinalBoneUpload.assign(totalBones, Matrix::Identity);

	const KeyFrameInfo* clipBuffer = mAniKeyFrame.data();
	const AnimationClipMeta* metaBuffer = mAniClipMeta.data();
	const uint32 metaCount = static_cast<uint32>(mAniClipMeta.size());
	const SkeletonBoneParams* boneBuffer = mBoneData.data();

	// 인스턴스별로 본을 연산 — 각자의 ReulstIndex 위치에 결과를 채움
	for (size_t idx = 0; idx < mAnimationPass.size(); ++idx)
	{
		const AnimationInstance& inst = mAnimationPass[idx];
		if (inst.BoneCount == 0)
			continue;

		mScratchModelBones.assign(inst.BoneCount, Matrix::Identity);

		// 본 평가 결과 저장 후 곧바로 업로드 형식(Transpose)으로 변환
		std::vector<Matrix> finalRowMajor(inst.BoneCount, Matrix::Identity);

		const AimParams* aimPtr = (idx < mAimPass.size()) ? &mAimPass[idx] : nullptr;
		AnimationEvaluator::Evaluate(
			inst, clipBuffer, metaBuffer, metaCount, boneBuffer,
			mScratchModelBones.data(), finalRowMajor.data(), aimPtr);

		Matrix* dst = &mFinalBoneUpload[inst.ReulstIndex];
		for (uint32 i = 0; i < inst.BoneCount; ++i)
		{
			dst[i] = finalRowMajor[i].Transpose();
		}

		std::vector<Matrix>& modelCache = mEntityModelBones[inst.EntityID];
		modelCache.assign(mScratchModelBones.begin(), mScratchModelBones.begin() + inst.BoneCount);
	}

	const uint32 uploadBytes = static_cast<uint32>(sizeof(Matrix) * mFinalBoneUpload.size());
	RENDERMANAGER.GetGroupBuffer(RENDERMANAGER.GetFrameResourceIndex())->AnimResultInfo
		->UpdateDefaultFromCpu(mFinalBoneUpload.data(), uploadBytes);
}

bool CpuAnimationSystem::GetModelBoneMatrix(Entity entity, uint32 boneIndex, Matrix& outMatrix) const
{
	// 엔티티별 모델 본 캐시에서 조회.
	auto it = mEntityModelBones.find(entity.GetID());
	if (it == mEntityModelBones.end())
		return false;
	if (boneIndex >= it->second.size())
		return false;
	// 캐시 없음/인덱스 범위 밖이면 false.

	outMatrix = it->second[boneIndex];
	return true;
}

void CpuAnimationSystem::AnimationBlend(const shared_ptr<Animator>& animClip, float updateTime,
	uint32& currentFrame, uint32& nextFrame, float& ratio)
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
