#pragma once
#include "System.h"

class World;

class DecalSystem : public System
{
public:
    explicit DecalSystem(World* world) : System(world) { mPhase = SysPhase::Sim; }

    void Update(float deltaTime) override;
};
