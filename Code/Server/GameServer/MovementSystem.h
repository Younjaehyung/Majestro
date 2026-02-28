#pragma once
#include "World.h"
#include "System.h"
#include <vector>
class MovementSystem :public System
{
public:
	MovementSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

	void RegisterActiveBullet(Entity bulletEntity);
	void UnregisterActiveBullet(Entity bulletEntity);

private:
	std::vector<EntityID> mActiveBulletEntityIds;

};

