#include "pch.h"
#include "ServerCore.h"
#include "World.h"
#include "GameRuleSystem.h"
#include "GameRuleComponent.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"
#include "TruckComponent.h"
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
			SendSceneConquest(rule);
			SendSceneEscort(rule);
		}



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

void GameNetRuleSystem::SendSceneConquest(Entity rule)
{
	
	if (auto* g = mWorld->GetComponent<GameConquestComponent>(rule))
	{
		S2C_ConquestPacket pkt{};
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
