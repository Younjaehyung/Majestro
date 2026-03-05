#pragma once
#include "System.h"

class DamageSystem : public System
{
public:
    explicit DamageSystem(World* world);

    void Update(float deltaTime) override;
};