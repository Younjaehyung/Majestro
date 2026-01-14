#pragma once
#include "World.h"
#include "System.h"

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

public:
	const float mDPI = 0.5f;
};