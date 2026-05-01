#pragma once
#include "World.h"
#include "System.h"

class GamePhaseSystem : public System
{
public:
	GamePhaseSystem(World* world) : System(world) { mPhase = SysPhase::Pre; }
	void Initialize();
	void Update(float deltaTime);
private:


};

