#include "pch.h"
#include "ServerCore.h"
#include "World.h"
#include "GameRuleSystem.h"
#include "GameRuleComponent.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"
#include "ScoreBoardComponent.h"
#include "TruckComponent.h"
#include "TransformComponent.h"
#include "Prefab.h"


GamePreRuleSystem::GamePreRuleSystem(World* world, shared_ptr<GameMode> gameMode) : System(world), mGameMode(gameMode)
{
	mPhase = SysPhase::Post;
}

void GamePreRuleSystem::Update(float deltaTime)
{
	if (mGameMode) {
		mGameMode->PreUpdate(deltaTime);
	}
}

GamePostRuleSystem::GamePostRuleSystem(World* world, shared_ptr<GameMode> gameMode) : System(world), mGameMode(gameMode)
{
}

void GamePostRuleSystem::Update(float deltaTime)
{
	

	if (mGameMode) {

		GameRuleComponent* ruleComp = mWorld->GetSingleton<GameRuleComponent>();
		if (ruleComp) ruleComp->mGameTime += deltaTime;


		mGameMode->PostUpdate(deltaTime);
	}
}

GameNetRuleSystem::GameNetRuleSystem(World* world, shared_ptr<GameMode> gameMode) : System(world), mGameMode(gameMode)
{
	mPhase = SysPhase::Post;
	mOrder = 100;

}

void GameNetRuleSystem::Update(float deltaTime)
{

	if (mGameMode) {

		CollectPlayerSessions();

        if (mSessionSet.empty()) return;

        Entity rule = mWorld->GetSingletonEntity();


		// 글로벌 — 항상 존재
		if (mSceneStateSendRate.Tick(deltaTime))
			SendSceneState(rule);


		// 활성 phase
		if (mScenePhaseSendRate.Tick(deltaTime)) {
			SendScenePrepare(rule);
			SendSceneConquest(rule);
			SendSceneEscort(rule);
			SendSceneClear(rule);
		}

		// The score board is replicated at a low fixed rate so newly joined clients receive it too.
		if (mScoreBoardSendRate.Tick(deltaTime))
			SendScoreBoard();

	}

}

void GameNetRuleSystem::SendSceneState(Entity rule)
{
	if (auto* s = mWorld->GetComponent<GameRuleComponent>(rule))
	{
		S2C_SceneStatePacket pkt{};
		pkt.GameTime = s->mGameTime;
		pkt.GamePhase = s->mGamePhase;
		pkt.PlayerScore = s->mPlayerScore;
		
		Broadcast(S2C_PKT_SCENE_STATE, pkt);
	}
}

void GameNetRuleSystem::SendScenePrepare(Entity rule)
{
	if (auto* prepare = mWorld->GetComponent<GamePrepareComponent>(rule))
	{
		S2C_PreparePacket pkt{};
		pkt.ReadyPlayers = prepare->mReadyPlayers;
		pkt.TotalPlayers = prepare->mTotalPlayers;
		pkt.ReadyCheckRemaining = prepare->mReadyCheckRemaining;
		pkt.ForcedStartRemaining = prepare->mForcedStartRemaining;
		pkt.CountdownRemaining = prepare->mCountdownRemaining;
		pkt.CountdownStarted = prepare->mCountdownStarted ? 1 : 0;
		pkt.AllPlayersReady = prepare->mAllPlayersReady ? 1 : 0;

		Broadcast(S2C_PKT_SCENE_PREPARE, pkt);
	}
}

void GameNetRuleSystem::SendSceneConquest(Entity rule)
{
	
	if (auto* g = mWorld->GetComponent<GameConquestComponent>(rule))
	{
		S2C_ConquestPacket pkt{};
		pkt.ActiveZoneId = static_cast<uint8>(g->mActiveZoneIndex + 1);

		// 구역 좌표 동봉
		const int32 zoneIdx = g->mActiveZoneIndex;
		if (zoneIdx >= 0 && zoneIdx < GameConquestComponent::mMaxWaves)
		{
			Entity zone = g->mConquestPointRect[zoneIdx];
			if (zone.IsValid())
			{
				if (auto* zoneTr = mWorld->GetComponent<TransformComponent>(zone))
				{
					pkt.ZoneX = zoneTr->mWorldPosition.x;
					pkt.ZoneY = zoneTr->mWorldPosition.y;
					pkt.ZoneZ = zoneTr->mWorldPosition.z;
				}
			}
		}

		pkt.WaveCheckPoint = g->mWaveCheckPoint;
		pkt.Wave = g->mWave;
		pkt.WaveInterval = g->mWaveInterval;
		pkt.WaveTime = g->mWaveTime;
		pkt.RequiredConquestTime = g->mRequiredConquestTime;
		pkt.PlayerNum = g->mPlayerNum;
		pkt.EnemyNum = g->mEnemyNum;

		Broadcast(S2C_PKT_SCENE_CONQUEST, pkt);
	}
}

void GameNetRuleSystem::SendSceneEscort(Entity rule)
{
	if (auto* e = mWorld->GetComponent<GameEscortComponent>(rule))
	{
		S2C_EscortPacket pkt{};
		pkt.RouteId = e->mRouteId;
		pkt.EscortStage = e->mEscortStage;
		pkt.MoveState = e->mMoveState;
		pkt.StageCount = e->mStageCount;
		pkt.EscortProgress = e->mEscortProgress;
		pkt.StageProgress = e->mStageProgress;
		pkt.EscortTime = e->mEscortTime;
		
		if (e->mEscortTarget.IsValid())
		{
			if (auto* net = mWorld->GetComponent<NetEntityComponent>(e->mEscortTarget))
				pkt.TruckNetId = net->mNetEntityId;
		}
		Broadcast(S2C_PKT_SCENE_ESCORT, pkt);
	}
}

void GameNetRuleSystem::SendSceneClear(Entity rule)
{
	if (auto* clear = mWorld->GetComponent<GameClearComponent>(rule))
	{
		S2C_ClearPacket pkt{};
		pkt.PlayerCount = clear->mPlayerCount;
		pkt.TeamScore = clear->mTeamScore;
		pkt.GameTime = clear->mGameTime;

		for (uint8 index = 0; index < clear->mPlayerCount && index < ROOM_MAX_PLAYERS; ++index)
		{
			pkt.Players[index].SessionId = clear->mSessionIds[index];
			pkt.Players[index].PlayerType = clear->mPlayerTypes[index];
		}

		Broadcast(S2C_PKT_SCENE_CLEAR, pkt);
	}
}

void GameNetRuleSystem::SendScoreBoard()
{
	if (!mWorld->HasComponentPool<NetEntityComponent>() ||
		!mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	const ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
	std::vector<ScoreBoardPlayerInfo> players;
	players.reserve(ROOM_MAX_PLAYERS);

	for (Entity entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
	{
		const NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		const MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);
		if (!netComp || !playerComp || netComp->mSessionId == 0)
			continue;

		ScoreBoardPlayerInfo info{};
		info.SessionId = netComp->mSessionId;
		info.PlayerType = static_cast<uint8>(playerComp->mPlayerType);

		if (scoreBoard)
		{
			if (const PlayerScoreStat* stat = scoreBoard->Find(netComp->mSessionId))
			{
				info.Score = stat->mScore;
				info.TotalKills = stat->mTotalKills;
			}
		}

		players.push_back(info);
	}

	// Keep packet order deterministic and show the highest score first.
	std::sort(players.begin(), players.end(), [](const ScoreBoardPlayerInfo& lhs, const ScoreBoardPlayerInfo& rhs)
	{
		if (lhs.Score != rhs.Score)
			return lhs.Score > rhs.Score;
		if (lhs.TotalKills != rhs.TotalKills)
			return lhs.TotalKills > rhs.TotalKills;
		return lhs.SessionId < rhs.SessionId;
	});

	S2C_ScoreBoardPacket pkt{};
	pkt.PlayerCount = static_cast<uint8>((std::min)(players.size(), static_cast<size_t>(ROOM_MAX_PLAYERS)));
	for (uint8 index = 0; index < pkt.PlayerCount; ++index)
		pkt.Players[index] = players[index];

	Broadcast(S2C_PKT_SCORE_BOARD, pkt);
}


void GameNetRuleSystem::CollectPlayerSessions()
{

	mSessionSet.clear();
	if (false == mWorld->HasComponentPool<NetEntityComponent>())
		return;

	auto entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	mSessionSet.reserve(entities.size());
	for (auto entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (!netComp || netComp->mSessionId == 0)
			continue;

		mSessionSet.insert(netComp->mSessionId);
	}
}
