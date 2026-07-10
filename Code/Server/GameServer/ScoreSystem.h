#pragma once
#include "System.h"

class ScoreSystem : public System
{
public:
    explicit ScoreSystem(World* world);

    void Update(float deltaTime) override;

private:
    void SampleMaxCombo();
    void ConsumeSupportActions(float now);
    void ConsumeEnemyKills(float now);
};
