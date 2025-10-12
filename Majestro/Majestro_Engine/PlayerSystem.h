#pragma once
#include "World.h"
#include "System.h"

class PlayerSystem : public System
{
public:
	PlayerSystem(World* world);

	void Initialize();
	void Update(float dt);
};
