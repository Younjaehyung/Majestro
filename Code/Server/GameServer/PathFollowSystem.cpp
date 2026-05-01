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
		if (!path->mActive) continue;
		if (!path->mPathData || !path->mPathData->IsValid()) continue;

		path->mPreviousDistance = path->mCurrentDistance;

		// StopPoint 대기 중이면 거리 진행 없이 타이머만 감소
		if (path->mPaused)
		{
			path->mWaitTimer -= dt;
			if (path->mWaitTimer <= 0.f)
			{
				path->mPaused = false;
				path->mWaitTimer = 0.f;
			}
		}
		else
		{
			path->mCurrentDistance = path->mPathData->AdvanceDistance(
				path->mCurrentDistance, dt, path->mBaseSpeed);
		}

		
		PayloadPathSample sample{};
		if (path->mPathData->Evaluate(path->mCurrentDistance, sample))
		{
			Vec3 prevPos = tr->mLocalPosition;

			tr->mLocalPosition = sample.position;
			tr->mMovingVector  = sample.position - prevPos;

			// forward(=Look) 방향으로 회전. up 은 LookAt 내부에서 Vec3::Up 기준으로 처리됨.
			// 시네마틱에서 롤이 필요해지면 sample.up 을 반영하도록 별도 경로로 확장.
			tr->LookAt(sample.forward);
		}

		// 일시정지 중에는 이벤트 발사 스킵 (정지 위치에서 매 프레임 동일 이벤트가 재발사되는 것 방지)
		if (path->mPaused) continue;

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

			std::cout << "[PathFollow] entity=" << entity.GetID()
			          << " event=" << ev->name
			          << " id=" << ev->eventId
			          << " distance=" << ev->distance << std::endl;
		}
	}
}
