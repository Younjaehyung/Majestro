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
	
	GameRuleComponent& rule = mScene->GetWorld()->AddSingleton<GameRuleComponent>();


	if (mHasCustomPhases)
	{
		// Scene에서 직접 지정한 초기 큐 사용 
		mPhaseQueue = std::move(mInitialPhases);
	}
	else
	{
		mPhaseQueue.push_back([] { return new PreparePhase(); });
		mPhaseQueue.push_back([] { return new EscortPhase(/*routeId=*/0); });
	}

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
	// 현재 phase 가 끝나지 않았으면 대기 (Escort 가 마지막 stop 통과 시 ClearPhase 등을 큐에 삽입한다)
	if (mCurrentPhase && false == mCurrentPhase->IsCompleted()) return;

	if (mPhaseQueue.empty())
	{
		// 현재 phase 까지 완료 + 큐 비었을 때 게임 종료/전환 (목적지는 씬이 지정)
		mIsComplete = true;
		mTargetSceneId = mCompletionSceneId;
		mIsSceneChanging = true;
		return;
	}

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
