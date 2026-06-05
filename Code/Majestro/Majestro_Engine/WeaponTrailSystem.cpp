#include "pch.h"
#include "WeaponTrailSystem.h"

#include "TransformComponent.h"
#include "TransformSystem.h"
#include "SocketComponent.h"
#include "SocketSystem.h"
#include "PlayerComponent.h"
#include "AnimationComponent.h"
#include "World.h"

WeaponTrailSystem::WeaponTrailSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> WeaponTrailSystem::After() const
{
	return { typeid(TransformSystem), typeid(SocketSystem) };
}

void WeaponTrailSystem::Update(float deltaTime)
{
	if (mWorld == nullptr || mWorld->HasComponentPool<WeaponTrailComponent>() == false)
		return;

	auto view = mWorld->View<WeaponTrailComponent>();
	for (Entity entity : view)
	{
		WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(entity);
		if (trail == nullptr || trail->mIsActive == false)
			continue;

		if (trail->mAutoActivateOnAttack)
		{
			const Entity src = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : entity;
			if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(src))
			{
				const int state = player->mUpperState;
				const bool attacking =
					state == static_cast<int>(ReplicatedActionState::Attack1) ||
					state == static_cast<int>(ReplicatedActionState::Attack2) ||
					state == static_cast<int>(ReplicatedActionState::Skill1) ||
					state == static_cast<int>(ReplicatedActionState::Skill2) ||
					state == static_cast<int>(ReplicatedActionState::ComboAttack1) ||
					state == static_cast<int>(ReplicatedActionState::ComboAttack2) ||
					state == static_cast<int>(ReplicatedActionState::Special);

				trail->mActive = attacking && IsAnimationWindowActive(src, *trail);
			}
		}

		UpdateTrail(entity, *trail, deltaTime);
	}
}

bool WeaponTrailSystem::IsAnimationWindowActive(Entity sourceEntity, const WeaponTrailComponent& trail) const
{
	if (trail.mUseAnimationWindow == false)
		return true;

	const AnimationComponent* animation = mWorld->GetComponent<AnimationComponent>(sourceEntity);
	if (animation == nullptr || animation->mAnimClips.empty())
		return true;

	const bool useUpper = trail.mUseUpperAnimationWindow && animation->mEnableUpperBodyLayer;
	const uint32 clipIndex = useUpper ? animation->mUpperAnimClipIdx : animation->mLowerAnimClipIdx;
	if (clipIndex >= animation->mAnimClips.size())
		return true;

	const shared_ptr<Animator>& clip = animation->mAnimClips[clipIndex];
	if (clip == nullptr)
		return true;

	const float duration = max(static_cast<float>(clip->mDuration), 0.0001f);
	const float animTime = std::clamp(useUpper ? animation->mUpperUpdateTime : animation->mUpdateTime, 0.0f, duration);

	float startTime = max(trail.mTrailStartTime, 0.0f);
	float endTime = trail.mTrailEndTime;
	if (trail.mUseAnimationFrameWindow)
	{

		const uint32 frameCount = max(clip->mClipMeta.NumFrame, 1u);
		const float frameDuration = duration / static_cast<float>(frameCount);
		const uint32 startFrame = min(trail.mTrailStartFrame, frameCount - 1u);
		const uint32 endFrame = min(trail.mTrailEndFrame, frameCount - 1u);

		startTime = static_cast<float>(startFrame) * frameDuration;
		endTime = min(static_cast<float>(endFrame + 1u) * frameDuration, duration);
	}

	if (endTime <= startTime)
		endTime = duration;

	endTime = std::clamp(endTime, 0.0f, duration);
	startTime = std::clamp(startTime, 0.0f, duration);
	return animTime >= startTime && animTime < endTime;
}

void WeaponTrailSystem::UpdateTrail(Entity entity, WeaponTrailComponent& trail, float deltaTime)
{
	AgeSamples(trail, deltaTime);

	if (trail.mActive == false)
	{
		trail.mWasActive = false;
		trail.mTimeSinceLastSample = 0.0f;
		return;
	}

	if (trail.mWasActive == false)
	{
		if (trail.mResetOnActivate)
			trail.mSamples.clear();

		trail.mTimeSinceLastSample = trail.mSampleInterval;
	}

	trail.mWasActive = true;
	trail.mTimeSinceLastSample += deltaTime;

	if (trail.mTimeSinceLastSample >= trail.mSampleInterval)
	{
		AddSample(entity, trail);
		trail.mTimeSinceLastSample = 0.0f;
	}
}

void WeaponTrailSystem::AgeSamples(WeaponTrailComponent& trail, float deltaTime)
{
	for (WeaponTrailSample& sample : trail.mSamples)
	{
		sample.Age += deltaTime;
	}

	const float lifetime = max(trail.mLifetime, 0.001f);
	std::erase_if(trail.mSamples, [lifetime](const WeaponTrailSample& sample)
		{
			return sample.Age >= lifetime;
		});
}

void WeaponTrailSystem::AddSample(Entity ownerEntity, WeaponTrailComponent& trail)
{
	const Entity sourceEntity = trail.mSourceEntity.IsValid() ? trail.mSourceEntity : ownerEntity;

	Vec3 tipWorld{};
	Vec3 baseWorld{};

	if (trail.mSourceType == WeaponTrailSource::Socket)
	{
		SocketComponent* socketComponent = mWorld->GetComponent<SocketComponent>(sourceEntity);
		if (socketComponent == nullptr)
			return;

		Matrix tipMatrix;
		Matrix baseMatrix;
		if (socketComponent->TryGetSocketWorldMatrix(trail.mTipSocketName, tipMatrix) == false)
			return;
		if (socketComponent->TryGetSocketWorldMatrix(trail.mBaseSocketName, baseMatrix) == false)
			return;

		tipWorld = tipMatrix.Translation();
		baseWorld = baseMatrix.Translation();
	}
	else
	{
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(sourceEntity);
		if (transform == nullptr)
			return;

		const Matrix& world = transform->GetWorldMatrix();
		tipWorld = Vec3::Transform(trail.mTipLocalOffset, world);
		baseWorld = Vec3::Transform(trail.mBaseLocalOffset, world);

		if ((tipWorld - baseWorld).LengthSquared() < 0.001f)
			baseWorld = tipWorld - transform->GetRight() * trail.mFallbackWidth;
	}

	if (trail.mSamples.empty() == false)
	{
		const WeaponTrailSample& last = trail.mSamples.back();
		const float tipDistance = (tipWorld - last.TipWorld).Length();
		const float baseDistance = (baseWorld - last.BaseWorld).Length();
		if (max(tipDistance, baseDistance) < trail.mMinSegmentDistance)
			return;
	}

	WeaponTrailSample sample{};
	sample.TipWorld = tipWorld;
	sample.BaseWorld = baseWorld;
	sample.Age = 0.0f;

	// WeaponTrail 이동추적
	if (trail.mFollowOwnerMovement)
	{
		const Entity referenceEntity = trail.mReferenceEntity.IsValid() ? trail.mReferenceEntity : sourceEntity;
		if (TransformComponent* refTransform = mWorld->GetComponent<TransformComponent>(referenceEntity))
		{
			Matrix referenceFrame = BuildWeaponTrailReferenceFrame(refTransform->mWorldMatrix, trail.mFollowReferenceRotation);
			Matrix inverseReference;
			referenceFrame.Invert(inverseReference);
			if (inverseReference != Matrix::Identity)
			{
				sample.TipLocal = Vec3::Transform(tipWorld, inverseReference);
				sample.BaseLocal = Vec3::Transform(baseWorld, inverseReference);
			}
			else
			{
				// 역행렬 실패시 월드 좌표를 그대로 두어 최소한 트레일이 랜더링
				sample.TipLocal = tipWorld;
				sample.BaseLocal = baseWorld;
			}
		}
	}

	trail.mSamples.push_back(sample);

	if (trail.mSamples.size() > WeaponTrailComponent::MAX_SAMPLES)
	{
		const size_t removeCount = trail.mSamples.size() - WeaponTrailComponent::MAX_SAMPLES;
		trail.mSamples.erase(trail.mSamples.begin(), trail.mSamples.begin() + removeCount);
	}
}
