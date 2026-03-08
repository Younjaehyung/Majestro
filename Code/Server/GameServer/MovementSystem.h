#pragma once
#include "World.h"
#include "System.h"


class NavigationSystem;
class NavMesh;


class MovementSystem :public System
{
public:
	MovementSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);


private:

};

