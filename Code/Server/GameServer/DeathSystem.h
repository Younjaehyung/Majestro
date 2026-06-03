#pragma once
#include "System.h"
#include "Entity.h"

class World;

class DeathSystem : public System
{
public:
    DeathSystem(World* world);

    void Update(float deltaTime) override;

private:

    void ConsumeEvent();

    void BeginFallDeath();
    void BeginNormalDeath();

    void CompletePlayerDeath(Entity entity);
    void RespawnPlayer(Entity entity);
    void RevivePlayerAtCurrentPosition(Entity entity);
    
    bool BeginPlayerDeath(World* world, Entity entity, PlayerDeathCause cause, float holdSeconds, bool sendHealthZeroEvent);

private:
    // 낙사 높이
    static constexpr float KILL_Y = -800.0f;

    // 낙사시 스폰까지 시간
    static constexpr float RESPAWN_DELAY = 2.0f;

    shared_ptr<EventManager> mEventManager;
};
