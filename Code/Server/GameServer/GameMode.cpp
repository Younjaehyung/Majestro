#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "World.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "TransformComponent.h"
#include "GameRuleComponent.h"
#include "EnemyComponent.h"
#include "GamePhase.h"
#include "ServerCore.h"



void LobbyGameMode::Initialize()
{

}

void LobbyGameMode::PostUpdate(float deltaTime)
{
}

//--------------------------------------------------------------

void WaveGameMode::Initialize()
{
	mGameRuleEntity = mScene->GetWorld()->CreateEntity();
	GameRuleComponent& rule = mScene->GetWorld()->AddComponent<GameRuleComponent>(mGameRuleEntity);


	mPhaseQueue.push_back([] { return new EscortPhase(/*routeId=*/0); });
	mPhaseQueue.push_back([] { return new BossPhase();                });
	
	auto factory = std::move(mPhaseQueue.front());
	mPhaseQueue.pop_front();
	TransitionTo(factory());
}

void WaveGameMode::PreUpdate(float deltaTime)
{
	mGameTime += deltaTime;
	AdvancePhase();

	mCurrentPhase->PreUpdate(deltaTime, *this);

}

void WaveGameMode::PostUpdate(float deltaTime)
{

	if (mScene == nullptr) return;

	mCurrentPhase->PostUpdate(deltaTime, *this);	
}

void WaveGameMode::TransitionTo(GamePhase* next)
{
	if (mCurrentPhase) mCurrentPhase->Exit(*this);
	mCurrentPhase = std::move(next);
	if (mCurrentPhase) mCurrentPhase->Enter(*this);
}

void WaveGameMode::AdvancePhase()
{
	if (mPhaseQueue.empty())
	{
		// 모든 phase 클리어 — 승리
		mIsComplete = true;
		mTargetSceneId = SceneId::VGame;
		mIsSceneChanging = true;
		return;
	}

	
	if (false == mCurrentPhase->IsCompleted()) return;

	auto factory = std::move(mPhaseQueue.front());
	mPhaseQueue.pop_front();
	TransitionTo(factory());


}

void WaveGameMode::InsertNextPhase(PhaseFactory factory)
{
	mPhaseQueue.push_front(std::move(factory));
}

//--------------------------------------------------------------

void ResultGameMode::Initialize()
{
}

void ResultGameMode::PreUpdate(float deltaTime)
{
}

void ResultGameMode::PostUpdate(float deltaTime)
{
}
