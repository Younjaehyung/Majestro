#pragma once
#include "World.h"
#include "System.h"
#include "BulletComponent.h"

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

private:
	bool EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType);

	SkillType ResolveSkillType(uint8 playerType, InputButtons actionButton, uint8 rhythm = 0);

	void EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo);

public:
	const float mDPI = 5.f;
};