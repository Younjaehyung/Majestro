#pragma once
#include "World.h"
#include "System.h"
class TransformSystem :public System
{
public:
	TransformSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

};

