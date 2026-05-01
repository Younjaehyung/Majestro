#pragma once
#include "pch.h"
#include "Component.h"
#include "Entity.h"

class GameRuleComponent : public Component<GameRuleComponent>
{
public:
    GameRuleComponent() = default;
   
	float mGameTime = 0.0f; // 게임 진행 시간
	int32 mPlayerScore = 0; // 플레이어 점수 (예: 점령 시간, 처치 수 등)
	uint8 mGamePhase = 0; // 현재 게임 phase (예: 준비, 점령, 호위 등)
};


class GameConquestComponent : public Component<GameConquestComponent>
{
public:
	GameConquestComponent() = default;
   
	// 상수

	static constexpr int mMaxWaves = 3; // 최대 웨이브 수

	static constexpr int mMaxWaveCheckPoint = 3; // 최대 웨이브 체크포인트 수
	static constexpr float mWaveCheckPointTime = 10.f; // 웨이브 체크포인트별 시간

	static constexpr float mMaxWaveInterval = 3.f; // 웨이브 점령 감소 간격 시작 (초)
	static constexpr float mMaxWaveTime = 30.f; // 최대 웨이브 수

	// 런타임
	int32 mWaveCheckPoint = 0; // 현재 웨이브 체크포인트 번호 (0부터 시작)
	int32 mWave = 1;          // 현재 웨이브 번호 (1부터 시작)

	float mWaveInterval = 0.0f; // 웨이브 점령 감소 간격 (초)
	float mWaveTime = 0.0f; // 웨이브 점령 시간

	int32 mPlayerNum = 0; // 플레이어 수
	int32 mEnemyNum = 0; // 적 수

};

class GameEscortComponent : public Component<GameEscortComponent>
{
public:
    GameEscortComponent() = default;
   
	Entity mEscortTarget; // 호위 대상 엔티티 (예: 트럭)

	float mEscortProgress = 0.f; // 호위 진행도 (0.0 ~ 1.0)
	float mEscortTime = 0.f; // 호위 진행 시간
};