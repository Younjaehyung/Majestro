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

    // 리스폰 위치에 스폰 VFX(EvEffectSpawn)를 방송한다. 실제 부활은 RESPAWN_VFX_LEAD초 뒤에 수행한다.
    void BeginRespawnVfx(Entity entity);

    bool BeginPlayerDeath(World* world, Entity entity, PlayerDeathCause cause, float holdSeconds, bool sendHealthZeroEvent);

private:
    // 낙사 높이
    static constexpr float KILL_Y = -800.0f;

    // 낙사시 스폰까지 시간
    static constexpr float RESPAWN_DELAY = 2.0f;

    // 리스폰 쿨타임이 끝난 뒤, 스폰 위치 VFX를 먼저 재생하고 실제 부활까지 기다리는 시간
    static constexpr float RESPAWN_VFX_LEAD = 1.0f;

    shared_ptr<EventManager> mEventManager;
};
