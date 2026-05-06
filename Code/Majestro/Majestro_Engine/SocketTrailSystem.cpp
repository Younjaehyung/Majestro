#include "pch.h"
#include "SocketTrailSystem.h"

#include "AnimationComponent.h"
#include "AnimationSystem.h"
#include "CpuAnimationSystem.h"
#include "NetInterpolationSystem.h"
#include "ParticleSystem.h"
#include "PlayerComponent.h"
#include "Skeleton.h"
#include "TransformComponent.h"
#include "TransformSystem.h"
#include "WeaponTrailComponent.h"
#include "World.h"

namespace
{
	shared_ptr<Skeleton> ResolveSkeleton(AnimationComponent* animationComponent)
	{
		if (animationComponent == nullptr || animationComponent->mAnimClips.empty())
			return nullptr;

		const uint32 clipIndex = min(animationComponent->mAnimClipIdx,
			static_cast<uint32>(animationComponent->mAnimClips.size() - 1));
		const shared_ptr<Animator>& clip = animationComponent->mAnimClips[clipIndex];
		return clip ? clip->GetSkeleton() : nullptr;
	}

	string ToLowerCopy(string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	uint32 FindBoneByKeyword(const Skeleton& skeleton, const std::vector<string>& keywords)
	{
		const auto& bones = skeleton.GetBones();
		for (const string& keyword : keywords)
		{
			for (uint32 i = 0; i < static_cast<uint32>(bones.size()); ++i)
			{
				const string lowerBoneName = ToLowerCopy(bones[i].boneName);
				if (lowerBoneName.find(keyword) != string::npos)
					return i;
			}
		}

		return Skeleton::INVALID_BONE_INDEX;
	}

	uint32 ResolveSocketBoneIndex(shared_ptr<Skeleton> skeleton, TrailSocketDesc& socket)
	{
		if (skeleton == nullptr)
			return Skeleton::INVALID_BONE_INDEX;

		if (socket.boneIndex != Skeleton::INVALID_BONE_INDEX)
			return socket.boneIndex;

		if (socket.boneName.empty() == false)
			socket.boneIndex = skeleton->FindBoneIndexByName(socket.boneName);

		if (socket.boneIndex == Skeleton::INVALID_BONE_INDEX)
		{
			// 수정: 아직 FBX의 정확한 무기 본 이름이 코드에 고정되어 있지 않으므로
			// weapon/mace/axe 계열 이름을 우선 찾고, 없으면 오른손 본을 임시 소켓으로 사용한다.
			static const std::vector<string> weaponKeywords =
			{
				"weapon", "mace", "axe", "hammer", "guitar", "blade", "sword"
			};
			socket.boneIndex = FindBoneByKeyword(*skeleton, weaponKeywords);
		}

		if (socket.boneIndex == Skeleton::INVALID_BONE_INDEX)
		{
			static const std::vector<string> handKeywords =
			{
				// 수정: 현재 캐릭터 skel 리소스는 무기 전용 본보다 "Bip001 L Hand" 손 본이 확인된다.
				// 무기 본을 못 찾으면 왼손/오른손 계열을 모두 fallback 후보로 사용한다.
				"l hand", "left hand", "hand_l", "lefthand", "left_hand", "hand.l", "l_hand",
				"r hand", "right hand", "hand_r", "righthand", "right_hand", "hand.r", "r_hand"
			};
			socket.boneIndex = FindBoneByKeyword(*skeleton, handKeywords);
		}

		return socket.boneIndex;
	}

	bool ResolveSocketWorldMatrix(
		World* world,
		Entity ownerEntity,
		TrailSocketDesc& socket,
		Matrix& outWorldMatrix)
	{
		if (world == nullptr || ownerEntity.IsValid() == false)
			return false;

		auto* animationComponent = world->GetComponent<AnimationComponent>(ownerEntity);
		auto* transformComponent = world->GetComponent<TransformComponent>(ownerEntity);
		if (animationComponent == nullptr || transformComponent == nullptr)
			return false;
		if (animationComponent->mModelBonePaletteValid == false)
			return false;

		shared_ptr<Skeleton> skeleton = ResolveSkeleton(animationComponent);
		const uint32 boneIndex = ResolveSocketBoneIndex(skeleton, socket);
		if (boneIndex == Skeleton::INVALID_BONE_INDEX)
			return false;
		if (boneIndex >= animationComponent->mModelBonePalette.size())
			return false;

		outWorldMatrix = socket.localOffset
			* animationComponent->mModelBonePalette[boneIndex]
			* transformComponent->GetWorldMatrix();
		return true;
	}

	Vec3 ResolveSocketWorldPosition(
		World* world,
		Entity ownerEntity,
		TrailSocketDesc& socket,
		bool& outResolved)
	{
		Matrix socketWorld = Matrix::Identity;
		outResolved = ResolveSocketWorldMatrix(world, ownerEntity, socket, socketWorld);
		return outResolved ? socketWorld.Translation() : Vec3::Zero;
	}

	bool IsAttackState(int state)
	{
		return state == S_Attack1 ||
			state == S_Attack2 ||
			state == S_Skill1 ||
			state == S_Skill2 ||
			state == S_Special;
	}

	bool IsInsideAttackWindow(const WeaponTrailComponent& trail, AnimationComponent* animationComponent)
	{
		if (trail.mUseAttackWindow == false)
			return true;
		if (animationComponent == nullptr || animationComponent->mAnimClips.empty())
			return true;

		const uint32 upperClipIndex = min(animationComponent->mUpperAnimClipIdx,
			static_cast<uint32>(animationComponent->mAnimClips.size() - 1));
		const shared_ptr<Animator>& upperClip = animationComponent->mAnimClips[upperClipIndex];
		if (upperClip == nullptr || upperClip->mDuration <= 0.0001)
			return true;

		const float duration = static_cast<float>(upperClip->mDuration);
		float normalized = fmodf(max(animationComponent->mUpperUpdateTime, 0.f), duration) / duration;
		normalized = std::clamp(normalized, 0.f, 1.f);

		const float start = std::clamp(trail.mAttackWindowStart, 0.f, 1.f);
		const float end = std::clamp(trail.mAttackWindowEnd, 0.f, 1.f);
		if (start <= end)
			return normalized >= start && normalized <= end;

		return normalized >= start || normalized <= end;
	}

	void PushTrailSample(WeaponTrailComponent& trail, const Vec3& basePos, const Vec3& tipPos)
	{
		TrailSample sample{};
		sample.basePos = basePos;
		sample.tipPos = tipPos;

		if (trail.mSamples.empty() == false)
		{
			const Vec3 prevCenter = (trail.mSamples.back().basePos + trail.mSamples.back().tipPos) * 0.5f;
			const Vec3 currCenter = (basePos + tipPos) * 0.5f;
			sample.distanceFromStart = trail.mSamples.back().distanceFromStart + Vec3::Distance(prevCenter, currCenter);
		}

		trail.mSamples.push_back(sample);
		while (trail.mSamples.size() > trail.mMaxSamples)
			trail.mSamples.pop_front();
	}

	void AppendTrailSamples(WeaponTrailComponent& trail, const Vec3& basePos, const Vec3& tipPos)
	{
		if (trail.mHasPreviousSample == false)
		{
			PushTrailSample(trail, basePos, tipPos);
			trail.mPreviousBasePos = basePos;
			trail.mPreviousTipPos = tipPos;
			trail.mHasPreviousSample = true;
			return;
		}

		const Vec3 prevCenter = (trail.mPreviousBasePos + trail.mPreviousTipPos) * 0.5f;
		const Vec3 currCenter = (basePos + tipPos) * 0.5f;
		const float moveDistance = Vec3::Distance(prevCenter, currCenter);
		if (moveDistance < trail.mMinSampleDistance)
			return;

		// 수정: 빠른 공격 모션에서도 trail이 끊기지 않도록 이전/현재 소켓 위치 사이를 보간해 샘플을 추가한다.
		const float step = max(trail.mInterpolationStep, 0.001f);
		const int splitCount = max(1, static_cast<int>(ceilf(moveDistance / step)));
		for (int i = 1; i <= splitCount; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(splitCount);
			const Vec3 lerpBase = Vec3::Lerp(trail.mPreviousBasePos, basePos, t);
			const Vec3 lerpTip = Vec3::Lerp(trail.mPreviousTipPos, tipPos, t);
			PushTrailSample(trail, lerpBase, lerpTip);
		}

		trail.mPreviousBasePos = basePos;
		trail.mPreviousTipPos = tipPos;
	}

	void UpdateTrailLife(WeaponTrailComponent& trail, float deltaTime)
	{
		for (TrailSample& sample : trail.mSamples)
			sample.age += deltaTime;

		while (trail.mSamples.empty() == false && trail.mSamples.front().age > trail.mLifeTime)
			trail.mSamples.pop_front();
	}
}

SocketTrailSystem::SocketTrailSystem(World* world)
	: System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> SocketTrailSystem::Before() const
{
	return { typeid(ParticleSystem) };
}

std::vector<std::type_index> SocketTrailSystem::After() const
{
	return
	{
		typeid(CpuAnimationSystem),
		typeid(GpuAnimationSystem),
		typeid(TransformSystem),
		typeid(NetInterpolationSystem)
	};
}

void SocketTrailSystem::Update(float deltaTime)
{
	if (mWorld == nullptr || mWorld->HasComponentPool<WeaponTrailComponent>() == false)
		return;

	for (Entity entity : mWorld->View<WeaponTrailComponent>())
	{
		auto* trail = mWorld->GetComponent<WeaponTrailComponent>(entity);
		if (trail == nullptr)
			continue;

		const Entity sourceEntity = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : entity;
		auto* player = mWorld->GetComponent<MainPlayerComponent>(sourceEntity);
		auto* animation = mWorld->GetComponent<AnimationComponent>(sourceEntity);

		if (trail->mAutoActivateFromPlayerState && player != nullptr)
			trail->mActive = IsAttackState(player->mStatePacket);

		bool baseResolved = false;
		bool tipResolved = false;
		const Vec3 basePos = ResolveSocketWorldPosition(mWorld, sourceEntity, trail->mBaseSocket, baseResolved);
		const Vec3 tipPos = ResolveSocketWorldPosition(mWorld, sourceEntity, trail->mTipSocket, tipResolved);

		const bool canEmit = trail->mActive &&
			IsInsideAttackWindow(*trail, animation) &&
			baseResolved &&
			tipResolved;

		if (canEmit)
			AppendTrailSamples(*trail, basePos, tipPos);
		else
			trail->mHasPreviousSample = false;

		UpdateTrailLife(*trail, deltaTime);
	}
}
