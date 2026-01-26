#include "pch.h"
#include "NetInterpolationSystem.h"
#include "World.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "NetTransformComponent.h"
#include "TagComponent.h"

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

		if (mWorld->GetComponent<LocalPlayerComponent>(entity)) {
			transform->mLocalPosition = Vec3::Lerp(netTransform->mStartPosition, netTransform->mTargetPosition, t);
			transform->mLocalRotation = Vec3::Lerp(netTransform->mStartRotation, netTransform->mTargetRotation, t);
		}
		else {
			transform->mLocalPosition = netTransform->mTargetPosition;
			transform->mLocalRotation = netTransform->mTargetRotation;
		}
		

		//PlayerMovementComponent* movementComponent = mWorld->GetComponent<PlayerMovementComponent>(entity);
		//if (movementComponent) {
		//	movementComponent->mCameraRotationX = transform->mLocalRotation.x;
		//	movementComponent->mCameraRotationY = transform->mLocalRotation.y;
		//}
			


		if (t >= 1.0f)
		{
			netTransform->mHasTarget = false;
		}
	}
}