#pragma once
#include "World.h"
#include "System.h"
class EnemySystem :public System
{
public:
	EnemySystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

};

