#pragma once
#include "System.h"

class BossTileGlowSystem : public System
{
public:
    explicit BossTileGlowSystem(World* world);

    void Update(float deltaTime) override;

private:
    void ConsumeTileEvents();
    void UpdateGlows(float deltaTime);
};
