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

		if (mWorld->GetComponent<LocalPlayerComponent>(entity))
		{
			// 로컬 플레이어: 서버 보정값으로 Lerp (새 패킷이 올 때만 mHasTarget = true)
			if (!netTransform->mHasTarget) continue;

			netTransform->mElapsed += dt;
			const float duration = (netTransform->mDuration <= 0.0f) ? 0.0001f : netTransform->mDuration;
			const float t = min(netTransform->mElapsed / duration, 1.0f);

			transform->mLocalPosition  = Vec3::Lerp(netTransform->mStartPosition, netTransform->mTargetPosition, t);
			transform->mLocalRotationE = Vec3::Lerp(netTransform->mStartRotation,  netTransform->mTargetRotation,  t);

			if (t >= 1.0f)
				netTransform->mHasTarget = false;
		}
		else
		{
			// 원격 엔티티: 스냅샷 버퍼 보간 + dead reckoning
			// mHasAnchor 이후 매 프레임 실행 — 패킷이 없는 구간도 외삽 계속
			if (!netTransform->mHasAnchor) continue;

			netTransform->UpdateRender(static_cast<double>(TIMER.GetTotalTime()));

			Vec3 rotE = netTransform->mRenderRotQ.ToEuler();
			transform->mLocalPosition  = netTransform->mRenderPos;
			transform->mLocalRotationE = {
				DirectX::XMConvertToDegrees(rotE.x),
				DirectX::XMConvertToDegrees(rotE.y),
				DirectX::XMConvertToDegrees(rotE.z)
			};
		}
	}
}
