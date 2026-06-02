#pragma once
#include "System.h"
#include "World.h"
#include "MovementSystem.h"
#include "CollisionSystem.h"


class PlayerNavValidationSystem : public System
{

public:
    PlayerNavValidationSystem(World* world);

    void Update(float dt) override;

    std::vector<std::type_index> After() const override { return { typeid(MovementSystem) }; }

private:
    // Jolt 위치에서 NavMesh 폴리곤을 탐색하는 반경
    static constexpr float kProjectRadius = 200.0f;
};
