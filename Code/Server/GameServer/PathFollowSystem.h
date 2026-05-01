#pragma once
#include "World.h"
#include "System.h"


class PathFollowSystem : public System
{
public:
	PathFollowSystem(World* world);

	void Initialize() {}
	void Update(float deltaTime);
};
