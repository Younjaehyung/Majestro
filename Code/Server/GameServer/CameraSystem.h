#pragma once
#include "World.h"
#include "System.h"

class CameraSystem : public System
{
public:
	CameraSystem(World* world);

	void Initialize();
	void Update(float dt);
	void TestUpdate(float dt);
};

