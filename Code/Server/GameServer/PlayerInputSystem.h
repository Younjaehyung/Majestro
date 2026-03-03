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
	void ActivateBulletAndNotify(Entity playerEntity, BulletType bulletType);
	std::vector<uint32> CollectPlayerSessions() const;

public:
	const float mDPI = 5.f;
};