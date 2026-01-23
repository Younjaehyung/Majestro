#pragma once
#include "World.h"
#include "System.h"

//class TransformComponent;
//class BoxColliderComponent;

class CollisionSystem :public System
{
public:
	CollisionSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

};

//static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);