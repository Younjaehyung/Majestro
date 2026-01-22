#include "pch.h"
#include "NetInterpolationSystem.h"
#include "World.h"
#include "TransformComponent.h"
#include "NetTransformComponent.h"

NetInterpolationSystem::NetInterpolationSystem(World* world) : System(world)
{
}

void NetInterpolationSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<NetTransformComponent>()) return;
	if (false == mWorld->HasComponentPool<TransformComponent>()) return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetTransformComponent, TransformComponent>();
	for (auto& entity : entities)
	{
		NetTransformComponent* netTransform = mWorld->GetComponent<NetTransformComponent>(entity);
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		if (!netTransform || !transform) continue;
		if (!netTransform->mHasTarget) continue;

		netTransform->mElapsed += dt;
		const float duration = (netTransform->mDuration <= 0.0f) ? 0.0001f : netTransform->mDuration;
		float t = netTransform->mElapsed / duration;
		if (t > 1.0f) t = 1.0f;

		transform->mLocalPosition = Vec3::Lerp(netTransform->mStartPosition, netTransform->mTargetPosition, t);
		transform->mLocalRotation = Vec3::Lerp(netTransform->mStartRotation, netTransform->mTargetRotation, t);

		if (t >= 1.0f)
		{
			netTransform->mHasTarget = false;
		}
	}
}