#include "pch.h"
#include "GamePhaseSystem.h"

#include "GameRuleComponent.h"


void GamePhaseSystem::Initialize()
{

}

void GamePhaseSystem::Update(float deltaTime)
{
	Entity e = mWorld->GetGameRuleEntity();
	
	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	if (!gameRuleComp) return;
	WavePhaseType currentPhase = static_cast<WavePhaseType>(gameRuleComp->mGamePhase);
	
	switch (currentPhase)
	{
	case WavePhaseType::Prepare:
		// 준비 단계 로직 (예: 웨이브 시작 대기, UI 업데이트 등)
		break;
	case WavePhaseType::Conquest:
		// 점령 단계 로직 (예: 점령 시간 감소, 점령 상태 체크 등)
	
		break;
	case WavePhaseType::Escort:
		// 호위 단계 로직 (예: 호위 대상 이동, 적 스폰 등)
		break;
	case WavePhaseType::Boss:
		// 보스 단계 로직 (예: 보스 행동 패턴, 체력 관리 등)
		break;
	case WavePhaseType::Fail:
		// 실패 단계 로직 (예: 게임 오버 처리, 리스타트 대기 등)
		break;
	case WavePhaseType::Clear:
		// 클리어 단계 로직 (예: 승리 처리, 다음 스테이지 준비 등)
		break;
	default:
		break;
	}

}