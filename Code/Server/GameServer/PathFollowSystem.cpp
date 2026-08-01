#include "pch.h"
#include "PathFollowSystem.h"

#include "TransformComponent.h"
#include "PathLoadComponent.h"
#include "PayloadPathData.h"


PathFollowSystem::PathFollowSystem(World* world) : System(world)
{
	mPhase = SysPhase::Sim;
	mOrder = 100;
}

void PathFollowSystem::Update(float dt)
{
	if (!mWorld->HasComponentPool<PathLoadComponent>()) return;
	if (!mWorld->HasComponentPool<TransformComponent>()) return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<PathLoadComponent>();
	for (auto& entity : entities)
	{
		PathLoadComponent*  path = mWorld->GetComponent<PathLoadComponent>(entity);
		TransformComponent* tr   = mWorld->GetComponent<TransformComponent>(entity);
		if (!path || !tr) continue;
		if (!path->mActive || path->mPaused) continue;
		if (!path->mPathData || !path->mPathData->IsValid()) continue;

		path->mPreviousDistance = path->mCurrentDistance;


		{
			path->mCurrentDistance = path->mPathData->AdvanceDistance(
				path->mCurrentDistance, dt, path->mBaseSpeed);
		}
		
		
		PayloadPathSample sample{};
		if (path->mPathData->Evaluate(path->mCurrentDistance, sample))
		{
			Vec3 prevPos = tr->mLocalPosition;

			tr->mLocalPosition = sample.position + path->mBaseOffset;
			tr->mMovingVector  = sample.position + path->mBaseOffset  - prevPos;

			// forward(=Look) 방향으로 회전. up 은 LookAt 내부에서 Vec3::Up 기준으로 처리됨.
			// 시네마틱에서 롤이 필요해지면 sample.up 을 반영하도록 별도 경로로 확장.
			tr->LookAt(sample.forward);
		}

		// 통과한 EventPoint 처리
		std::vector<const PayloadEventPoint*> passedEvents;
		path->mPathData->CollectPassedEvents(
			path->mPreviousDistance, path->mCurrentDistance, passedEvents);

		for (const PayloadEventPoint* ev : passedEvents)
		{
			if (ev->fireOnce)
			{
				if (path->mFiredEvents.contains(ev->name)) continue;
				path->mFiredEvents.insert(ev->name);
			}

			MJLOG_INFO(CombatDetail, "경로 이벤트 entity={} event={} id={} distance={}", entity.GetID(), ev->name, ev->eventId, ev->distance);
		}
	}
}
