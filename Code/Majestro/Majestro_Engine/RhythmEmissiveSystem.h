#pragma once
#include "System.h"

class RhythmEmissiveSystem : public System
{
public:
    explicit RhythmEmissiveSystem(World* world);

    void Update(float deltaTime) override;

private:
    void UpdateEmissives(float deltaTime);
};
