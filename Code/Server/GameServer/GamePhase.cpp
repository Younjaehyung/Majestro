#include "pch.h"
#include "GamePhase.h"
#include "GameCore.h"
#include "ResourceManager.h"	

#include "GameRuleComponent.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "TransformComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"
#include "InteractableComponent.h"
#include "PhysicsWorld.h"
#include "PayloadPathData.h"
#include "PathLoadComponent.h"
#include "NetEntityComponent.h"
#include "TruckComponent.h"

#include "Prefab.h"

void PreparePhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mGameRuleEntity = mode.GetGameRuleEntity();
}

void PreparePhase::Exit(WaveGameMode& mode)
{
	//mWorld->RemoveComponent<GameRuleComponent>(mGameRuleEntity);
}

void PreparePhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void PreparePhase::PostUpdate(float dt, WaveGameMode& mode)
{
	// 준비 단계에서는 플레이어가 준비를 완료했는지 체크하는 로직
	// 예: 모든 플레이어가 준비 상태가 되었는지 확인하고, 준비가 완료되면 mIsCompleted를 true로 설정하여 다음 단계로 전환
}

////--------------------------------------------------------------



void ConquestPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mGameRuleEntity = mode.GetGameRuleEntity();
	GameRuleComponent* ruleComp = mWorld->GetComponent<GameRuleComponent>(mGameRuleEntity);


	GameConquestComponent& conquestComp = mWorld->AddComponent<GameConquestComponent>(mGameRuleEntity);
	ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Conquest);
	conquestComp.mActiveZoneIndex = min(GameConquestComponent::mMaxWaves - 1, max(0, static_cast<int32>(mZoneId) - 1));
	conquestComp.mRequiredConquestTime = max(0.1f, mRequiredSeconds);

	// 씬에 배치된 ConquestZone 트리거를 mValueA(점령지 번호) 기준으로 wave 슬롯에 매핑
	if (mWorld->HasComponentPool<InteractableComponent>())
	{
		for (Entity e : mWorld->GetEntitiesWithComponents<InteractableComponent, TransformComponent, BoxColliderComponent>())
		{
			auto* inter = mWorld->GetComponent<InteractableComponent>(e);
			if (!inter || inter->mKind != InteractableKind::ConquestZone) continue;

			const int32 idx = static_cast<int32>(inter->mValueA) - 1;
			if (idx < 0 || idx >= GameConquestComponent::mMaxWaves) continue;

			conquestComp.mConquestPointRect[idx] = e;
		}
	}
}

void ConquestPhase::Exit(WaveGameMode& mode)
{

	mWorld->RemoveComponent<GameConquestComponent>(mGameRuleEntity);

}

void ConquestPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void ConquestPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	int playerNum = 0;
	int enemyNum = 0;

	GameConquestComponent* ruleComp = mWorld->GetComponent<GameConquestComponent>(mGameRuleEntity);
	if (!ruleComp) return;

	const int32 idx = ruleComp->mActiveZoneIndex;
	if (idx >= 0 && idx < GameConquestComponent::mMaxWaves)
	{
		Entity zone = ruleComp->mConquestPointRect[idx];
		if (zone.IsValid())
		{
			TransformComponent*  zoneTr  = mWorld->GetComponent<TransformComponent>(zone);
			BoxColliderComponent* zoneCol = mWorld->GetComponent<BoxColliderComponent>(zone);
			if (zoneTr && zoneCol)
			{
				PhysicsWorld::UpdateWorldOBB(zoneTr, zoneCol);
				const auto& zoneOBB = zoneCol->mWorldOBB;

				auto countInZone = [&](Entity e, int& counter)
				{
					if (auto* hp = mWorld->GetComponent<HealthComponent>(e))
					{
						if (hp->IsDead()) return;
					}
					auto* userCol = mWorld->GetComponent<BoxColliderComponent>(e);
					if (!userCol) return;
					if (zoneOBB.Intersects(userCol->mWorldOBB)) counter++;
				};

				if (mWorld->HasComponentPool<MainPlayerComponent>())
				{
					for (Entity e : mWorld->GetEntitiesWithComponents<MainPlayerComponent, BoxColliderComponent>())
						countInZone(e, playerNum);
				}
				if (mWorld->HasComponentPool<EnemyComponent>())
				{
					for (Entity e : mWorld->GetEntitiesWithComponents<EnemyComponent, BoxColliderComponent>())
						countInZone(e, enemyNum);
				}
			}
		}
	}

	if (playerNum >= enemyNum) {

		if (playerNum > enemyNum) {
			ruleComp->mWaveTime += dt;
		}
		ruleComp->mWaveInterval = 0.f;
		std::cout << "[ConquestZone] :  PlayerNum: " << playerNum << ", EnemyNum: " << enemyNum << ", WaveTime: " << ruleComp->mWaveTime << std::endl;
	}
	else {
		// 웨이브 점령 감소 간격이 최대 간격보다 작으면 간격 증가, 
		// 그렇지 않으면 간격 초기화 및 웨이브 점령 시간 감소
		if (ruleComp->mMaxWaveInterval > ruleComp->mWaveInterval) {
			ruleComp->mWaveInterval += dt;
		}
		else {
			ruleComp->mWaveInterval = 0.f;

			if (ruleComp->mWaveTime > ruleComp->mWaveTime / float(ruleComp->mWaveCheckPoint))
				ruleComp->mWaveTime -= dt;
		}
		ruleComp->mWaveTime = max(0.f, ruleComp->mWaveTime);
	}

	ruleComp->mPlayerNum = playerNum;
	ruleComp->mEnemyNum = enemyNum;
	


	if (ruleComp->mWaveTime > ruleComp->mWaveCheckPointTime * float(ruleComp->mWaveCheckPoint) && ruleComp->mWaveCheckPoint < ruleComp->mMaxWaveCheckPoint) {
		ruleComp->mWaveCheckPoint += 1;
	}


	// 웨이브 점령 시간이 최대 웨이브 시간보다 크면 웨이브 증가
	if (ruleComp->mWaveTime >= ruleComp->mRequiredConquestTime) {
		mIsCompleted = true;
		return;
	}

	if (ruleComp->mWaveTime > ruleComp->mMaxWaveTime) {
		ruleComp->mWave += 1;
		ruleComp->mWaveTime = 0.f;
		ruleComp->mWaveCheckPoint = 0;
		ruleComp->mWaveInterval = 0.f;
	}

}
////--------------------------------------------------------------

EscortPhase::EscortPhase(uint8 routeId) : mRouteId(routeId) 
{
	switch (routeId)
	{
	case 0:
	case 1:
		mEscortPath = RESOURCEMANAGER.Get<PayloadPathData>(L"BP_Payroad_path_C_2_PayloadPath");
		break;
	default:
		// 로그/assert
		break;
	}

}

EscortPhase::EscortPhase(uint8 routeId, float startDistance, int32 nextStopIndex)
	: EscortPhase(routeId)
{
	// 이전 ConquestPhase가 끝난 stopPoint 거리부터 EscortPhase를 재개
	mStartDistance = startDistance;
	mNextStopIndex = nextStopIndex;
	mUseResumeDistance = true;
}


void EscortPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	Entity rule = mode.GetGameRuleEntity();
	GameRuleComponent* ruleComp = mWorld->GetComponent<GameRuleComponent>(rule);
	ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Escort);


	auto& state = mWorld->AddComponent<GameEscortComponent>(rule);
	state.mRouteId = mRouteId;


	GameEscortComponent* escortComp = mWorld->GetComponent<GameEscortComponent>(rule);
	escortComp->mEscortStage = static_cast<uint8>(mNextStopIndex);
	EntityView pathEntity = mWorld->View<PathLoadComponent, TruckComponent>();

	for( Entity e : pathEntity) {
		TruckComponent* truck = mWorld->GetComponent<TruckComponent>(e);
		if (truck) {
			escortComp->mEscortTarget = e;

			break;
		}
	}

	if (!escortComp->mEscortTarget.IsValid())
		return;

	PathLoadComponent* pathComp = mWorld->GetComponent<PathLoadComponent>(escortComp->mEscortTarget);
	pathComp->mPathData         = mEscortPath;
	pathComp->mBaseSpeed        = escortComp->mTruckSpeed;
	const float startDistance = mUseResumeDistance ? mStartDistance : 0.f;
	pathComp->mCurrentDistance  = startDistance;
	pathComp->mTotalDistance = mEscortPath ? mEscortPath->GetLength() : 0.f;
	pathComp->mPreviousDistance = startDistance;
	pathComp->mActive           = true;
	pathComp->mPaused           = true; // 첫 프레임 PostUpdate 가 반경 검사 후 풀어줌
	if (!mUseResumeDistance)
		pathComp->mFiredEvents.clear();

	// 재개 Phase 진입 순간 트럭 위치를 복구해서 경로 시작점으로 되돌아가지 않게 한다.
	TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(escortComp->mEscortTarget);
	if (targetTr && mEscortPath)
	{
		PayloadPathSample sample{};
		if (mEscortPath->Evaluate(startDistance, sample))
		{
			targetTr->mLocalPosition = sample.position + pathComp->mBaseOffset;
			targetTr->LookAt(sample.forward);
		}
	}
}





void EscortPhase::Exit(WaveGameMode& mode)
{
	mWorld->RemoveComponent<GameEscortComponent>(mode.GetGameRuleEntity());
}

void EscortPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void EscortPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	Entity rule = mode.GetGameRuleEntity();
	GameEscortComponent* ruleComp = mWorld->GetComponent<GameEscortComponent>(rule);
	if (!ruleComp)
		return;

	ruleComp->mEscortTime += dt;

	Entity escortTarget = ruleComp->mEscortTarget;
	if (!escortTarget.IsValid()) return;

	TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(escortTarget);
	PathLoadComponent*  pathComp = mWorld->GetComponent<PathLoadComponent>(escortTarget);
	if (!targetTr || !pathComp) return;

	// 트럭 반경 내 살아있는 플레이어 / 적 직접 카운트 (XZ 평면, mEscortRange 기준)
	const Vec3  truckPos = targetTr->mLocalPosition;
	const float r2       = ruleComp->mEscortRange * ruleComp->mEscortRange;

	int32 playerNum = 0;
	int32 enemyNum  = 0;

	auto countInRange = [&](Entity e, int32& counter)
	{
		if (auto* hp = mWorld->GetComponent<HealthComponent>(e))
		{
			if (hp->IsDead()) return;
		}
		auto* tr = mWorld->GetComponent<TransformComponent>(e);
		if (!tr) return;

		Vec3 d = tr->mLocalPosition - truckPos;
		d.y = 0.f; // XZ 거리만 비교
		if (d.LengthSquared() <= r2) counter++;
	};

	if (mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (Entity e : mWorld->GetEntitiesWithComponents<MainPlayerComponent, TransformComponent>())
			countInRange(e, playerNum);
	}
	if (mWorld->HasComponentPool<EnemyComponent>())
	{
		for (Entity e : mWorld->GetEntitiesWithComponents<EnemyComponent, TransformComponent>())
			countInRange(e, enemyNum);
	}

	ruleComp->mPlayerNum = playerNum;
	ruleComp->mEnemyNum  = enemyNum;

	// 반경 내 플레이어가 있고 적이 없을 때만 진행
	pathComp->mPaused = !(enemyNum == 0 && playerNum > 0);


	ruleComp->mMoveState = pathComp->mPaused ? 0 : 1;

	ruleComp->mEscortProgress = (pathComp->mTotalDistance > 0.f)
		? pathComp->mCurrentDistance / pathComp->mTotalDistance : 0.f;

	if (mEscortPath && mNextStopIndex >= 0)
	{
		const auto& stopPoints = mEscortPath->GetStopPoints();
		if (static_cast<size_t>(mNextStopIndex) < stopPoints.size())
		{
			const PayloadStopPoint& stop = stopPoints[mNextStopIndex];
			if (mEscortPath->DidPassDistance(pathComp->mPreviousDistance, pathComp->mCurrentDistance, stop.distance))
			{
				// stopPoint에 도달했으므로 ConquestPhase와 같은 거리에서 재개할 EscortPhase를 바로 다음 순서에 삽입
				pathComp->mCurrentDistance = stop.distance;
				pathComp->mPreviousDistance = stop.distance;
				pathComp->mPaused = true;

				PayloadPathSample sample{};
				if (mEscortPath->Evaluate(stop.distance, sample))
				{
					targetTr->mLocalPosition = sample.position + pathComp->mBaseOffset;
					targetTr->LookAt(sample.forward);
				}

				const uint8 routeId = mRouteId;
				const float resumeDistance = stop.distance;
				const int32 nextStopIndex = mNextStopIndex + 1;
				const uint8 zoneId = ResolveConquestZoneIdFromResumeEvent(stop.resumeEvent, nextStopIndex);
				const float requiredSeconds = (stop.waitSeconds > 0.f) ? stop.waitSeconds : GameConquestComponent::mMaxWaveTime;

				mode.InsertNextPhase([routeId, resumeDistance, nextStopIndex] {
					return new EscortPhase(routeId, resumeDistance, nextStopIndex);
				});
				mode.InsertNextPhase([zoneId, requiredSeconds] {
					return new ConquestPhase(zoneId, requiredSeconds);
				});

				ruleComp->mEscortStage = static_cast<uint8>(nextStopIndex);
				ruleComp->mEscortProgress = (pathComp->mTotalDistance > 0.f)
					? pathComp->mCurrentDistance / pathComp->mTotalDistance
					: 0.f;
				mIsCompleted = true;
				return;
			}
		}
	}

	if (ruleComp->mEscortProgress >= 1.f)
		mIsCompleted = true;
}

uint8 EscortPhase::ResolveConquestZoneIdFromResumeEvent(const std::string& resumeEvent, int32 fallbackZoneId)
{
	int32 value = 0;
	int32 multiplier = 1;
	bool foundDigit = false;

	for (auto it = resumeEvent.rbegin(); it != resumeEvent.rend(); ++it)
	{
		const char ch = *it;
		if (ch >= '0' && ch <= '9')
		{
			value += (ch - '0') * multiplier;
			multiplier *= 10;
			foundDigit = true;
			continue;
		}

		if (foundDigit)
			break;
	}

	// resumeEvent가 CheckPoint1 형식이면 숫자로 점령 구역을 고르고, 숫자가 없으면 stopPoint 순서를 사용
	const int32 zoneId = foundDigit ? value : fallbackZoneId;
	return static_cast<uint8>(min(255, max(1, zoneId)));
}

////--------------------------------------------------------------



void BossPhase::Enter(WaveGameMode& mode)
{
}

void BossPhase::Exit(WaveGameMode& mode)
{
}

void BossPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void BossPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	// 보스의 체력이나 행동 패턴을 체크하여 보스가 처치되었는지 판단하는 로직을 구현할 수 있습니다.
}
