#include "pch.h"
#include "NetInterpolationSystem.h"
#include "World.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "NetTransformComponent.h"
#include "TagComponent.h"
#include "Engine.h"
#include "Timer.h"

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
			transform->mLocalRotationE = Vec3::Lerp(netTransform->mStartRotation, netTransform->mTargetRotation, t);
		}
		else {

			NetSnapshot snapshot;
			snapshot.pos = netTransform->mTargetPosition;
			snapshot.rotQ = netTransform->mTargetRotationQ; // 또는 Euler로 변환해서 저장
			
			snapshot.serverTick = netTransform->mLastSequence; // 또는 실제 서버 시간
            snapshot.vel = netTransform->mVelocity;
			netTransform->SetServerHz(60.0);
			netTransform->SetSnapshotRateHz(20.0);
			netTransform->OnSnapshot(snapshot, netTransform->mLastUpdateTime);
			netTransform->UpdateRender(TIMER.GetTotalTime());
			Vec3 rotE = netTransform->mRenderRotQ.ToEuler();
			
			transform->mLocalPosition = netTransform->mRenderPos;
			transform->mLocalRotationE = { DirectX::XMConvertToDegrees(rotE.x),DirectX::XMConvertToDegrees(rotE.y),DirectX::XMConvertToDegrees(rotE.z) };
		 
			
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
