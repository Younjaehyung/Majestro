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
#include "GameEvents.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "PlayerSpawnComponent.h"
#include "NavMeshLoader.h"

#include "Prefab.h"

namespace
{
	void ApplyPrepareSpawnPosition(World* world, Entity playerEntity)
	{
		if (!world || !world->HasComponentPool<PlayerSpawnComponent>())
			return;

		MainPlayerComponent* player = world->GetComponent<MainPlayerComponent>(playerEntity);
		TransformComponent* transform = world->GetComponent<TransformComponent>(playerEntity);
		if (!player || !transform)
			return;

		const uint8 playerType = player->mPlayerType;
		if (playerType >= PlayerType::Count)
			return;

		auto spawnEntities = world->GetEntitiesWithComponent<PlayerSpawnComponent>();
		if (spawnEntities.empty())
			return;

		PlayerSpawnComponent* spawn =
			world->GetComponent<PlayerSpawnComponent>(spawnEntities[0]);
		if (!spawn)
			return;

		Vec3 spawnPosition = spawn->mCharacterPositions[playerType];

		// FirstScene처럼 지정 위치가 NavMesh에서 벗어난 경우 가장 가까운 이동 가능 지점으로 보정한다.
		shared_ptr<Navigation>& navigation = world->GetNavSystem();
		if (navigation && navigation->IsInitialized())
		{
			constexpr float spawnSearchRadius = 1000.0f;
			if (navigation->IsPointOnNavMesh(spawnPosition, spawnSearchRadius))
				spawnPosition = navigation->GetNearestPointOnNavMesh(spawnPosition, spawnSearchRadius);
		}

		transform->mLocalPosition = spawnPosition;
		transform->mWorldPosition = spawnPosition;
		transform->mWorldMatrix = Matrix::CreateTranslation(spawnPosition);
		transform->mMovingVector = Vec3::Zero;
		player->mSpawnPosition = spawnPosition;

		if (PlayerMovementComponent* movement =
			world->GetComponent<PlayerMovementComponent>(playerEntity))
		{
			movement->mMovingDirection = Vec3::Zero;
			movement->mNavPosition = spawnPosition;
			movement->mNavPositionValid = true;
		}

		if (GravityComponent* gravity = world->GetComponent<GravityComponent>(playerEntity))
		{
			gravity->mHight = spawnPosition.y;
			gravity->mGround = spawnPosition.y;
			gravity->mGravity = 0.0f;
			gravity->mFalling = false;
			gravity->mDropping = false;
			gravity->mGroundGraceLeft = 0.0f;
		}
	}
}

void PreparePhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();

	// PreparePhase 재진입 시 이전 준비 상태가 남지 않도록 모든 값을 초기화한다.
	mIsCompleted = false;
	mStartCount = 0.0f;
	mStartCountDown = 0.0f;
	mCountdownStarted = false;
	mReadyPlayers = 0;
	mTotalPlayers = 0;
	mPositionedPlayers.clear();

	if (GameRuleComponent* ruleComp = mWorld->GetSingleton<GameRuleComponent>())
		ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Prepare);

	// Prepare UI 동기화에 사용할 서버 상태를 생성하고 초기값을 기록한다.
	GamePrepareComponent& prepareComp = mWorld->AddSingleton<GamePrepareComponent>();
	prepareComp.mReadyPlayers = 0;
	prepareComp.mTotalPlayers = 0;
	prepareComp.mReadyCheckRemaining = kRequiredReadyTime;
	prepareComp.mForcedStartRemaining = kMaxReadyTime;
	prepareComp.mCountdownRemaining = 0.0f;
	prepareComp.mCountdownStarted = false;
	prepareComp.mAllPlayersReady = false;

	mode.GetScene()->ApplyPhaseSpawnerSet("Prepare");
}

void PreparePhase::Exit(WaveGameMode& mode)
{
	// 다음 Phase에서 Prepare UI 상태가 남지 않도록 전용 상태를 제거한다.
	if (mWorld)
		mWorld->RemoveSingleton<GamePrepareComponent>();
}

void PreparePhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void PreparePhase::PostUpdate(float dt, WaveGameMode& mode)
{
	if (mIsCompleted || !mWorld)
		return;

	// PreparePhase 중 새로 생성된 플레이어를 캐릭터별 지정 위치에 한 번만 강제 배치한다.
	if (mWorld->HasComponentPool<NetEntityComponent>() &&
		mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (Entity entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			if (!netComp || netComp->mNetEntityId == 0)
				continue;

			if (mPositionedPlayers.insert(netComp->mNetEntityId).second)
				ApplyPrepareSpawnPosition(mWorld.get(), entity);
		}
	}

	// 디버그 F10 을 누르면 준비 단계를 즉시 종료하고 다음 Phase 로 전환한다.
	const bool skipPressed = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
	if (skipPressed && !mSkipKeyHeld)
	{
		std::cout << "[PreparePhase] F10 입력 - 준비 단계 즉시 전환" << std::endl;
		mSkipKeyHeld = skipPressed;
		mIsCompleted = true;
		return;
	}
	mSkipKeyHeld = skipPressed;

	mStartCount += max(0.0f, dt);

	// 게임 Scene에는 별도 Ready 플래그가 없으므로 방 세션의 플레이어 엔티티가
	// 현재 World에 모두 생성된 상태를 해당 플레이어의 준비 완료로 판단한다.
	std::unordered_set<uint32> spawnedSessions;
	if (mWorld->HasComponentPool<NetEntityComponent>() &&
		mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (Entity entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			if (netComp && netComp->mSessionId != 0)
				spawnedSessions.insert(netComp->mSessionId);
		}
	}

	mReadyPlayers = static_cast<int32>(spawnedSessions.size());
	mTotalPlayers = 0;

	if (gGameCore && !spawnedSessions.empty())
	{
		const uint32 anySessionId = *spawnedSessions.begin();
		if (RoomState* room = gGameCore->GetRoomManager().GetRoomByPlayer(anySessionId))
		{
			const std::vector<uint64> roomSessions = room->GetSessionIds();
			mTotalPlayers = static_cast<int32>(roomSessions.size());
			mReadyPlayers = 0;

			for (uint64 sessionId : roomSessions)
			{
				if (spawnedSessions.count(static_cast<uint32>(sessionId)) > 0)
					++mReadyPlayers;
			}
		}
	}

	if (!mCountdownStarted)
	{
		const bool allPlayersReady = mTotalPlayers > 0 && mReadyPlayers >= mTotalPlayers;
		const bool forcedStart = mStartCount >= kMaxReadyTime;

		// 인원 준비 여부와 무관하게 입장 후 30초가 지나면 3초 카운트다운을 시작한다.
		if (forcedStart)
		{
			mCountdownStarted = true;
			mStartCountDown = kStartCountdownDuration;
		}

		if (GamePrepareComponent* prepareComp = mWorld->GetSingleton<GamePrepareComponent>())
		{
			prepareComp->mReadyPlayers = mReadyPlayers;
			prepareComp->mTotalPlayers = mTotalPlayers;
			prepareComp->mReadyCheckRemaining = max(0.0f, kRequiredReadyTime - mStartCount);
			prepareComp->mForcedStartRemaining = max(0.0f, kMaxReadyTime - mStartCount);
			prepareComp->mCountdownRemaining = mStartCountDown;
			prepareComp->mCountdownStarted = mCountdownStarted;
			prepareComp->mAllPlayersReady = allPlayersReady;
		}
		return;
	}

	mStartCountDown = max(0.0f, mStartCountDown - max(0.0f, dt));
	if (GamePrepareComponent* prepareComp = mWorld->GetSingleton<GamePrepareComponent>())
	{
		prepareComp->mReadyPlayers = mReadyPlayers;
		prepareComp->mTotalPlayers = mTotalPlayers;
		prepareComp->mReadyCheckRemaining = 0.0f;
		prepareComp->mForcedStartRemaining = max(0.0f, kMaxReadyTime - mStartCount);
		prepareComp->mCountdownRemaining = mStartCountDown;
		prepareComp->mCountdownStarted = true;
		prepareComp->mAllPlayersReady = mTotalPlayers > 0 && mReadyPlayers >= mTotalPlayers;
	}

	if (mStartCountDown <= 0.0f)
		mIsCompleted = true;

}

////--------------------------------------------------------------



void ConquestPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	GameRuleComponent* ruleComp = mWorld->GetSingleton<GameRuleComponent>();


	GameConquestComponent& conquestComp = mWorld->AddSingleton<GameConquestComponent>();
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

			// Map_Gimmicks.json 의 valueB(점령 시간)가 지정돼 있으면 이번 활성 구역의 필요 점령 시간을 덮어쓴다.
			if (idx == conquestComp.mActiveZoneIndex && inter->mValueB > 0.f)
				conquestComp.mRequiredConquestTime = inter->mValueB;
		}
	}

	// 점령지 번호별 스포너 세트
	mode.GetScene()->ApplyPhaseSpawnerSet("Conquest:" + std::to_string(static_cast<int32>(mZoneId)));
}

void ConquestPhase::Exit(WaveGameMode& mode)
{

	mWorld->RemoveSingleton<GameConquestComponent>();

}

void ConquestPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void ConquestPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	int playerNum = 0;
	int enemyNum = 0;

	GameConquestComponent* ruleComp = mWorld->GetSingleton<GameConquestComponent>();
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

	// 호위 구간별 스포너 세트 적용
	mode.GetScene()->ApplyPhaseSpawnerSet("Escort:" + std::to_string(mNextStopIndex));

	GameRuleComponent* ruleComp = mWorld->GetSingleton<GameRuleComponent>();
	ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Escort);


	auto& state = mWorld->AddSingleton<GameEscortComponent>();
	state.mRouteId = mRouteId;


	GameEscortComponent* escortComp = mWorld->GetSingleton<GameEscortComponent>();
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
	mWorld->RemoveSingleton<GameEscortComponent>();
}

void EscortPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void EscortPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	GameEscortComponent* ruleComp = mWorld->GetSingleton<GameEscortComponent>();
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

	// 인원 비례 속도
	{
		float multiplier = 1.0f;
		const int32 fullCount = (ruleComp->mFullSpeedPlayerCount > 1) ? ruleComp->mFullSpeedPlayerCount : 1;
		if (playerNum >= 1 && fullCount > 1)
		{
			const int32 clamped = (playerNum < fullCount) ? playerNum : fullCount; // 1..fullCount
			const float t = static_cast<float>(clamped - 1) / static_cast<float>(fullCount - 1); // 0..1
			multiplier = 1.0f + (ruleComp->mMaxSpeedMultiplier - 1.0f) * t;
		}
		ruleComp->mSpeedMultiplier = multiplier;
		pathComp->mBaseSpeed = ruleComp->mTruckSpeed * multiplier;
	}

	ruleComp->mEscortProgress = (pathComp->mTotalDistance > 0.f)
		? pathComp->mCurrentDistance / pathComp->mTotalDistance : 0.f;

	// 현재 구간 내 진행도(0~1)와 전체 구간 수를 계산해 UI 등분 리맵
	auto computeStageProgress = [&](int32 stageIdx)
	{
		if (!mEscortPath)	// 경로 없음
		{
			ruleComp->mStageCount = 0;
			ruleComp->mStageProgress = ruleComp->mEscortProgress;
			return;
		}
		const auto& stops = mEscortPath->GetStopPoints();
		ruleComp->mStageCount = static_cast<uint8>(stops.size());


		if (stops.empty())	// 중간 포인트 없음
		{
			ruleComp->mStageProgress = ruleComp->mEscortProgress;
			return;
		}
		stageIdx = std::clamp(stageIdx, 0, static_cast<int32>(stops.size()) - 1);
		const float segStart = (stageIdx == 0) ? 0.f : stops[stageIdx - 1].distance;
		const float segEnd   = stops[stageIdx].distance;
		const float segLen   = segEnd - segStart;
		ruleComp->mStageProgress = (segLen > 0.f)
			? std::clamp((pathComp->mCurrentDistance - segStart) / segLen, 0.f, 1.f) : 1.f;
	};

	computeStageProgress(mNextStopIndex);

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


				// 마지막 stop 이면 재개 호위 대신 ClearPhase 를 넣어 마지막 Conquest 직후 GameClear 및 씬 전환.
				const bool isLastStop = (static_cast<size_t>(mNextStopIndex) + 1 >= stopPoints.size());
				if (isLastStop)
				{
					mode.InsertNextPhase([] {
						return new ClearPhase(3.0f);
					});
				}
				else
				{
					mode.InsertNextPhase([routeId, resumeDistance, nextStopIndex] {
						return new EscortPhase(routeId, resumeDistance, nextStopIndex);
					});
				}
				mode.InsertNextPhase([zoneId, requiredSeconds] {
					return new ConquestPhase(zoneId, requiredSeconds);
				});

				ruleComp->mEscortStage = static_cast<uint8>(nextStopIndex);
				ruleComp->mEscortProgress = (pathComp->mTotalDistance > 0.f)
					? pathComp->mCurrentDistance / pathComp->mTotalDistance : 0.f;

				// 거점 통과 순간
				computeStageProgress(nextStopIndex);

				// 화물이 VFX
				if (auto* eventManager = mWorld->GetEventManager().get())
				{
					eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
						0, // effectType 미사용
						targetTr->mLocalPosition.x,
						targetTr->mLocalPosition.y,
						targetTr->mLocalPosition.z,
						EffectSpawnReason::CheckpointReached });
				}

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



void ClearPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mElapsed = 0.0f;
	mIsCompleted = false;

	if (auto* rule = mWorld->GetSingleton<GameRuleComponent>())
		rule->mGamePhase = static_cast<uint8>(WavePhaseType::Clear);

	// Clear UI가 사용할 플레이어 목록과 팀 결과를 현재 World에서 수집한다.
	GameClearComponent& clearComp = mWorld->AddSingleton<GameClearComponent>();
	clearComp = GameClearComponent{};

	if (GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>())
	{
		clearComp.mTeamScore = rule->mPlayerScore;
		clearComp.mGameTime = rule->mGameTime;
	}

	if (mWorld->HasComponentPool<NetEntityComponent>() &&
		mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (Entity entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			if (clearComp.mPlayerCount >= ROOM_MAX_PLAYERS)
				break;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);
			if (!netComp || !playerComp || netComp->mSessionId == 0)
				continue;

			const uint8 index = clearComp.mPlayerCount++;
			clearComp.mSessionIds[index] = netComp->mSessionId;
			clearComp.mPlayerTypes[index] = static_cast<uint8>(playerComp->mPlayerType);
		}
	}

	mode.GetScene()->ApplyPhaseSpawnerSet("Clear");
}

void ClearPhase::Exit(WaveGameMode& mode)
{
	// 다음 Phase 또는 Scene에 Clear 결과가 남지 않도록 전용 상태를 제거한다.
	if (mWorld)
		mWorld->RemoveSingleton<GameClearComponent>();
}

void ClearPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	mElapsed += dt;

	if (GameClearComponent* clearComp = mWorld->GetSingleton<GameClearComponent>())
	{
		if (GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>())
		{
			clearComp->mTeamScore = rule->mPlayerScore;
			clearComp->mGameTime = rule->mGameTime;
		}
	}

	// 유지 시간이 0 이하인 ClearPhase는 최종 결과 화면을 현재 게임 씬에 계속 유지한다.
	// 양수인 기존 스테이지 ClearPhase만 지정 시간 이후 다음 씬으로 진행한다.
	if (mHoldSeconds > 0.0f && mElapsed >= mHoldSeconds)
		mIsCompleted = true;
}

////--------------------------------------------------------------



void FailPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mElapsed = 0.0f;
	mIsCompleted = false;

	// 클라이언트가 GameOver 배너를 띄우도록 Fail phase 로 표시한다.
	if (auto* rule = mWorld->GetSingleton<GameRuleComponent>())
		rule->mGamePhase = static_cast<uint8>(WavePhaseType::Fail);

	// 전원 사망 후에는 적 스폰을 멈춘다. (정의되지 않은 키 → 모든 phase 관리 스포너 비활성)
	mode.GetScene()->ApplyPhaseSpawnerSet("Fail");
}

void FailPhase::Exit(WaveGameMode& mode)
{
}

void FailPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	mElapsed += dt;

	// 배너를 잠시 보여준 뒤 완료 → AdvancePhase 가 빈 큐를 보고 씬 전환(MainMenu)을 요청한다.
	if (mElapsed >= mHoldSeconds)
		mIsCompleted = true;
}

////--------------------------------------------------------------



void BossPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mIsCompleted = false;
	mBossDetected = false;


	if (auto* rule = mWorld->GetSingleton<GameRuleComponent>())
		rule->mGamePhase = static_cast<uint8>(WavePhaseType::Boss);

	mode.GetScene()->ApplyPhaseSpawnerSet("Boss:" + std::to_string(static_cast<int32>(mZoneId)));
}

void BossPhase::Exit(WaveGameMode& mode)
{
}

void BossPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void BossPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	(void)dt;
	(void)mode;

	if (mIsCompleted || !mWorld)
		return;

	if (!mWorld->HasComponentPool<EnemyComponent>() ||
		!mWorld->HasComponentPool<HealthComponent>())
		return;

	bool brassBossExists = false;
	for (Entity entity : mWorld->GetEntitiesWithComponents<EnemyComponent, HealthComponent>())
	{
		EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(entity);
		HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity);
		if (!enemy || !health || enemy->mEnemyType != EnemyType::Brass)
			continue;

		brassBossExists = true;
		mBossDetected = true;

		// Brass 보스 체력이 0 이하가 되면 BossPhase를 완료한다.
		// 다음 서버 틱에서 기존 Phase 큐의 ClearPhase가 시작되고 클라이언트에 Clear 상태가 전송된다.
		if (health->IsDead())
		{
			mIsCompleted = true;
			return;
		}
	}

	// 보스를 확인한 뒤 사망 처리로 엔티티가 제거된 경우도 처치 완료로 판정한다.
	if (mBossDetected && !brassBossExists)
		mIsCompleted = true;

}
