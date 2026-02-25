#pragma once
#include "World.h"
#include "System.h"

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

private:
	void ActivateBulletAndNotify(Entity playerEntity);
	std::vector<uint32> CollectPlayerSessions() const;

public:
	const float mDPI = 5.f;
};