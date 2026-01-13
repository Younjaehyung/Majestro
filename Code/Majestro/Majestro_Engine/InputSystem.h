#pragma once
#include "World.h"
#include "System.h"

class InputSystem : public System
{
public:
	InputSystem(World* world);

	void Initialize();
	void Update(float dt);

public:
	const float mDPI = 0.5f;
};