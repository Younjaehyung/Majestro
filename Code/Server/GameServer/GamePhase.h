#pragma once
#include "GameMode.h"
#include "Scene.h"
#include <unordered_set>

class World;
class PayloadPathData;

// 플레이어를 씬의 스폰 지점(PlayerSpawnComponent)으로 배치하고 NavMesh 보정/중력 초기화까지 수행.
void ApplyPlayerSpawnPosition(World* world, Entity playerEntity);

// Clear/Fail 결과 화면 유지 시간(초).
constexpr float kResultDecisionHoldSeconds = 15.0f;

class GamePhase
{
public:

    virtual ~GamePhase() = default;
   
    virtual void Enter(WaveGameMode& mode) {}
    virtual void Exit(WaveGameMode& mode) {}
    virtual void PreUpdate(float dt, WaveGameMode& mode) {}
    virtual void PostUpdate(float dt, WaveGameMode& mode) {}
    virtual bool IsCompleted() const = 0;            // 다음 phase로
    virtual uint8 GetType() const = 0;

protected:
	bool mIsCompleted = false;
    shared_ptr<World> mWorld;
};


class PreparePhase : public GamePhase
{
public:
	PreparePhase() = default;
	
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Prepare); }

private:
	constexpr static float kRequiredReadyTime = 5.f; // Prepare 진입 후 전원 준비 판정을 허용하는 최소 대기 시간
	constexpr static float kMaxReadyTime = 30.f; // 전원 준비 여부와 관계없이 카운트다운을 시작하는 최대 대기 시간
	constexpr static float kStartCountdownDuration = 3.f; // 다음 Phase로 전환하기 전 카운트다운 시간

	float mStartCount = 0.f; // PreparePhase 진입 후 경과 시간
	float mStartCountDown = 0.f; // 카운트다운 시작 후 남은 시간
	bool mCountdownStarted = false; // 전원 준비 또는 제한 시간으로 카운트다운이 시작됐는지 여부

	int32 mReadyPlayers = 0; // 현재 World에 플레이어 엔티티 생성이 완료된 방 인원 수
	int32 mTotalPlayers = 0; // 현재 방에 소속된 전체 세션 수

	bool mSkipKeyHeld = false; // [디버그] F10 즉시 시작 키 엣지 감지용
	std::unordered_set<uint64> mPositionedPlayers;
};

class ConquestPhase : public GamePhase
{
public:
    ConquestPhase(uint8 zoneId, float requiredSeconds = 30.f, uint8 waveIndex = 1)
        : mZoneId(zoneId), mRequiredSeconds(requiredSeconds), mWaveIndex(waveIndex) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Conquest); }

private:
	uint8 mZoneId = 0;
	float mRequiredSeconds = 30.f;
	uint8 mWaveIndex = 1; // 이 ConquestPhase가 HUD에서 채울 점령 링 번호(1-based). SecondScene 3링 순차 진행용
};

class EscortPhase : public GamePhase
{
 public:
     EscortPhase(uint8 routeId);
     EscortPhase(uint8 routeId, float startDistance, int32 nextStopIndex);
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Escort); }

private:

    uint8 ResolveConquestZoneIdFromResumeEvent(const std::string& resumeEvent, int32 fallbackZoneId);

private:
	Entity mTruckEntity;
    shared_ptr<PayloadPathData> mEscortPath;
private:
    uint8 mRouteId = 0;
	float mStartDistance = 0.f;
	int32 mNextStopIndex = 0;
	bool mUseResumeDistance = false;
	// RailPathComponent* mRailPath = nullptr; // 호위 경로 정보 (예: 웨이브 점령과 달리, 호위는 특정 경로를 따라 이동해야 할 수 있음)
};

class ClearPhase : public GamePhase
{
public:
    ClearPhase(float holdSeconds = 3.f) : mHoldSeconds(holdSeconds) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Clear); }

private:
    float mHoldSeconds = 3.f; // GameClear 배너를 띄워두는 시간
    float mElapsed = 0.f;
};

class FailPhase : public GamePhase
{
public:
    FailPhase(float holdSeconds = kResultDecisionHoldSeconds) : mHoldSeconds(holdSeconds) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Fail); }

private:
    float mHoldSeconds = 3.f; // GameOver 배너를 띄워두는 시간
    float mElapsed = 0.f;
};

class BossPhase : public GamePhase
{
public:
	BossPhase() = default;
	BossPhase(uint8 zoneId) : mZoneId(zoneId) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Boss); }


private:
    uint8 mZoneId = 0;

    bool mBossDetected = false;

};
