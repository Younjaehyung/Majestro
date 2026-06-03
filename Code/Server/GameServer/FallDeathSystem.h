#pragma once
#include "System.h"
#include "Entity.h"

class World;

// 낙사 시스템
class FallDeathSystem : public System
{
public:
    explicit FallDeathSystem(World* world);

    void Update(float deltaTime) override;

private:
    void RespawnPlayer(Entity entity);

    // 킬 플레인 높이. 이 값보다 transform.y 가 작아지면 낙사.
    static constexpr float KILL_Y = -800.0f;

    // 플레이어 사망 연출 시작 후 리스폰까지 지연(초).
    static constexpr float RESPAWN_DELAY = 2.0f;
};
