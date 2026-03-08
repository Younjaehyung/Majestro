#pragma once
#include "System.h"
#include "World.h"
#include "MovementSystem.h"
#include "CollisionSystem.h"


class PlayerNavValidationSystem : public System
{// 플레이어의 이번 프레임 XZ 이동을 NavMesh raycast로 검증해 벽 통과를 차단
// Y축(점프/낙하)은 GravitySystem에 위임하므로 검증 X
public:
    PlayerNavValidationSystem(World* world);

    void Update(float dt) override;

    std::vector<std::type_index> After()  const override { return { typeid(MovementSystem) }; }
    std::vector<std::type_index> Before() const override { return { typeid(CollisionSystem) }; }

private:
    // 이동 벡터 XZ 성분이 이 값 미만이면 검증 스킵
    static constexpr float MIN_MOVE_SQ = 0.01f * 0.01f; // cm 단위
};
