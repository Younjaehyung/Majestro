#pragma once
#include "World.h"
#include "System.h"
class MovementSystem :public System
{
public:
	MovementSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

};

