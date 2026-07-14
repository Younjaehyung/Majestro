#pragma once
#include "System.h"
#include "Entity.h"

class World;

class BuffAuraSystem : public System
{
public:
    explicit BuffAuraSystem(World* world) : System(world) { mPhase = SysPhase::Sim; }

    void Update(float deltaTime) override;

private:
    std::unordered_map<EntityID, Entity> mAuras;
};
