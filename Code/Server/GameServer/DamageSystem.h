#pragma once
#include "System.h"

class EventManager;

class DamageSystem : public System
{
public:
    explicit DamageSystem(World* world);

    void Update(float deltaTime) override;

private:
    void ApplyRudwigCriticalRhythmEffect(Entity instigator, EventManager& eventManager);
	void AddRhythmPoints(Entity player, int32 amount);
};
