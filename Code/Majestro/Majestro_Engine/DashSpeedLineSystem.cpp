#include "pch.h"
#include "DashSpeedLineSystem.h"

#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "TransformSystem.h"
#include "World.h"

DashSpeedLineSystem::DashSpeedLineSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> DashSpeedLineSystem::After() const
{
	return { typeid(TransformSystem) };
}

void DashSpeedLineSystem::Update(float deltaTime)
{
	if (mWorld == nullptr || mWorld->HasComponentPool<DashSpeedLineComponent>() == false)
		return;

	auto view = mWorld->View<DashSpeedLineComponent>();
	for (Entity entity : view)
	{
		DashSpeedLineComponent* trail = mWorld->GetComponent<DashSpeedLineComponent>(entity);
		if (trail == nullptr || trail->mIsActive == false)
			continue;

		if (trail->mAutoActivateOnDash)
		{
			const Entity src = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : entity;
			if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(src))
			{
				trail->mActive = player->mLowerState == static_cast<int>(ReplicatedMovementMode::Dashing);
			}
		}

		UpdateTrail(entity, *trail, deltaTime);
	}
}

void DashSpeedLineSystem::UpdateTrail(Entity entity, DashSpeedLineComponent& trail, float deltaTime)
{
	AgeSamples(trail, deltaTime);

	if (trail.mActive == false)
	{
		trail.mWasActive = false;
		trail.mTimeSinceLastSample = 0.0f;
		trail.mHasLastSourcePosition = false;
		return;
	}

	if (trail.mWasActive == false)
	{
		trail.mSamples.clear();
		trail.mTimeSinceLastSample = trail.mSampleInterval;
		trail.mHasLastSourcePosition = false;
	}

	trail.mWasActive = true;
	trail.mTimeSinceLastSample += deltaTime;

	if (trail.mTimeSinceLastSample >= trail.mSampleInterval)
	{
		AddSample(entity, trail);
		trail.mTimeSinceLastSample = 0.0f;
	}
}

void DashSpeedLineSystem::AgeSamples(DashSpeedLineComponent& trail, float deltaTime)
{
	for (DashSpeedLineSample& sample : trail.mSamples)
	{
		sample.Age += deltaTime;
	}

	const float lifetime = max(trail.mLifetime, 0.001f);
	std::erase_if(trail.mSamples, [lifetime](const DashSpeedLineSample& sample)
		{
			return sample.Age >= lifetime;
		});
}

void DashSpeedLineSystem::AddSample(Entity ownerEntity, DashSpeedLineComponent& trail)
{
	const Entity sourceEntity = trail.mSourceEntity.IsValid() ? trail.mSourceEntity : ownerEntity;
	TransformComponent* transform = mWorld->GetComponent<TransformComponent>(sourceEntity);
	if (transform == nullptr)
		return;


	const Vec3 sourcePosition = transform->mParent.IsValid() ? transform->GetWorldPosition() : transform->mLocalPosition;

	Vec3 direction = transform->GetLook();
	if (trail.mHasLastSourcePosition)
	{
		const Vec3 movement = sourcePosition - trail.mLastSourcePosition;
		if (movement.LengthSquared() > 0.001f)
			direction = movement;
	}

	direction.y = 0.0f;
	if (direction.LengthSquared() <= 0.001f)
		direction = Vec3::Forward;
	direction.Normalize();

	Vec3 right = Vec3::Up.Cross(direction);
	if (right.LengthSquared() <= 0.001f)
		right = transform->GetRight();
	right.y = 0.0f;
	if (right.LengthSquared() <= 0.001f)
		right = Vec3::Right;
	right.Normalize();

	DashSpeedLineSample sample{};
	sample.DirectionWorld = direction;
	sample.RightWorld = right;
	sample.CenterWorld = sourcePosition + Vec3::Up * trail.mHeightOffset - direction * trail.mBackOffset;
	sample.Age = 0.0f;

	trail.mLastSourcePosition = sourcePosition;
	trail.mHasLastSourcePosition = true;

	if (trail.mSamples.empty() == false)
	{
		const DashSpeedLineSample& last = trail.mSamples.back();
		if ((sample.CenterWorld - last.CenterWorld).Length() < trail.mMinSegmentDistance)
			return;
	}

	trail.mSamples.push_back(sample);

	if (trail.mSamples.size() > DashSpeedLineComponent::MAX_SAMPLES)
	{
		const size_t removeCount = trail.mSamples.size() - DashSpeedLineComponent::MAX_SAMPLES;
		trail.mSamples.erase(trail.mSamples.begin(), trail.mSamples.begin() + removeCount);
	}
}
