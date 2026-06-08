#pragma once
#include "GameMode.h"
#include "Scene.h"

class World;
class PayloadPathData;

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
    Entity mGameRuleEntity;
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
};

class ConquestPhase : public GamePhase
{
public:
    ConquestPhase(uint8 zoneId, float requiredSeconds = 30.f) : mZoneId(zoneId), mRequiredSeconds(requiredSeconds) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Conquest); }

private:
	uint8 mZoneId = 0;
	float mRequiredSeconds = 30.f;
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
    virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Clear); }

private:
    float mHoldSeconds = 3.f; // GameClear 배너를 띄워두는 시간
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

};
