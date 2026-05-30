#include "pch.h"
#include "WeaponTrailSystem.h"

#include "TransformComponent.h"
#include "TransformSystem.h"
#include "SocketComponent.h"
#include "SocketSystem.h"
#include "PlayerComponent.h"
#include "MovementComponent.h"
#include "World.h"

WeaponTrailSystem::WeaponTrailSystem(World* world): System(world)
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


		// 공격 상태 자동 활성화
		if (trail->mAutoActivateOnAttack)
		{
			const Entity src = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : entity;
			if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(src))
			{
				const int s = player->mUpperState;
				const bool attacking =
					s == static_cast<int>(ReplicatedActionState::Attack1) ||
					s == static_cast<int>(ReplicatedActionState::Attack2) ||
					s == static_cast<int>(ReplicatedActionState::Skill1) ||
					s == static_cast<int>(ReplicatedActionState::Skill2) ||
					s == static_cast<int>(ReplicatedActionState::Special);
				trail->mActive = attacking;
			}
		}

		UpdateTrail(entity, *trail, deltaTime);
	}
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
		// 소켓 모드 
		
		SocketComponent* socketComponent = mWorld->GetComponent<SocketComponent>(sourceEntity);
		if (socketComponent == nullptr)
			return;

		Matrix tipMatrix;
		Matrix baseMatrix;

		// 소켓 조회 실패(SocketComponent 없음 / 본 미해석 / GPU 애니 경로)면 이번 프레임 샘플을 추가 x
		if (socketComponent->TryGetSocketWorldMatrix(trail.mTipSocketName, tipMatrix) == false)
			return;
		if (socketComponent->TryGetSocketWorldMatrix(trail.mBaseSocketName, baseMatrix) == false)
			return;

		tipWorld = tipMatrix.Translation();
		baseWorld = baseMatrix.Translation();
	}
	else
	{
		// 기존 Transform 모드 
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(sourceEntity);
		if (transform == nullptr)
			return;

		const Matrix& world = transform->GetWorldMatrix();
		tipWorld = Vec3::Transform(trail.mTipLocalOffset, world);
		baseWorld = Vec3::Transform(trail.mBaseLocalOffset, world);

		if ((tipWorld - baseWorld).LengthSquared() < 0.001f)
		{
			baseWorld = tipWorld - transform->GetRight() * trail.mFallbackWidth;
		}
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
	trail.mSamples.push_back(sample);

	if (trail.mSamples.size() > WeaponTrailComponent::MAX_SAMPLES)
	{
		const size_t removeCount = trail.mSamples.size() - WeaponTrailComponent::MAX_SAMPLES;
		trail.mSamples.erase(trail.mSamples.begin(), trail.mSamples.begin() + removeCount);
	}
}
