#pragma once
#include "System.h"

class World;

class InteractionSystem : public System
{
public:
    InteractionSystem(World* world);

    void Initialize() override {}
    void Update(float deltaTime) override;

private:


    void ApplyHealPack(Entity user, Entity trigger, const class InteractableComponent& i);
    void ApplyArmorPack(Entity user, Entity trigger, const class InteractableComponent& i);
    void ApplyJumpPad(Entity user, Entity trigger, const class InteractableComponent& i);
    void ApplySpeedPad(Entity user, Entity trigger, const class InteractableComponent& i);
    void ApplyDamageZone(Entity user, Entity trigger, class InteractableComponent& i);
	void ApplyConquestZone(Entity user, Entity trigger, class InteractableComponent& i);

    bool MatchesTargetMask(World* world, Entity e, uint8 mask);

private:
    std::vector<Entity> candidates;
	int mPlayerCount = 0;
	int mEnemyCount = 0;
};
