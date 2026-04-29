#pragma once
#include "GameMode.h"
#include "Scene.h"

class World;


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
    ConquestPhase(uint8 zoneId) : mZoneId(zoneId) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Conquest); }

private:
	uint8 mZoneId = 0;
};

class EscortPhase : public GamePhase
{
 public:
	 EscortPhase(uint8 routeId) : mRouteId(routeId) {}
    virtual void Enter(WaveGameMode& mode) override;
    virtual void Exit(WaveGameMode& mode) override;
    virtual void PreUpdate(float dt, WaveGameMode& mode) override;
	virtual void PostUpdate(float dt, WaveGameMode& mode) override;
    virtual bool IsCompleted() const override { return mIsCompleted; }
    virtual uint8 GetType() const override { return static_cast<uint8>(WavePhaseType::Escort); }


private:
    uint8 mRouteId = 0;
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
