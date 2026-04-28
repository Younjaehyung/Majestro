#include "pch.h"
#include "ServerCore.h"
#include "World.h"
#include "GameRuleSystem.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"


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
		mGameMode->SendSceneState(deltaTime, std::vector<uint32>(mSessionSet.begin(), mSessionSet.end()));
	}
	
}
void GameNetRuleSystem::SendSceneState(float deltaTime)
{

	

	

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