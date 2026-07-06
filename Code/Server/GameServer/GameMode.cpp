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

	// 전원 사망(와이프) 감지 시 한 번만 GameOver(Fail) phase 로 전환한다.
	if (!mFailed && ShouldTriggerGameOver())
		TriggerGameOver();

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
		// 전원 사망(GameOver)으로 끝난 경우엔 광장으로 복귀해 재도전한다. (진행도는 유지)
		mIsComplete = true;
		mTargetSceneId = mFailed ? SceneId::Plaza : mCompletionSceneId;
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

bool WaveGameMode::ShouldTriggerGameOver() const
{
	if (mScene == nullptr || mCurrentPhase == nullptr)
		return false;

	// 전투가 진행되는 phase 에서만 와이프를 판정한다. (Prepare/Clear/Fail 등은 제외)
	const WavePhaseType phase = static_cast<WavePhaseType>(mCurrentPhase->GetType());
	if (phase != WavePhaseType::Conquest &&
		phase != WavePhaseType::Escort &&
		phase != WavePhaseType::Boss)
		return false;

	World* world = mScene->GetWorld().get();
	if (world == nullptr || !world->HasComponentPool<MainPlayerComponent>())
		return false;

	int32 playerCount = 0;
	for (Entity entity : world->GetEntitiesWithComponent<MainPlayerComponent>())
	{
		MainPlayerComponent* player = world->GetComponent<MainPlayerComponent>(entity);
		if (!player)
			continue;

		++playerCount;
		// 한 명이라도 살아있으면 와이프가 아니다.
		if (!player->IsDeathActive())
			return false;
	}

	// 플레이어가 한 명 이상 있고, 그 전원이 사망 상태일 때만 GameOver.
	return playerCount > 0;
}

void WaveGameMode::TriggerGameOver()
{
	mFailed = true;

	// 예약된 다음 phase 들을 비워 FailPhase 완료 후 곧바로 씬 전환(MainMenu)되게 한다.
	mPhaseQueue.clear();

	// 현재 phase 를 정리(Exit)하고 GameOver 배너 phase 로 진입.
	TransitionTo(new FailPhase());
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
