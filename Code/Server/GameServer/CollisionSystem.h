#pragma once
#include "World.h"
#include "System.h"

class TransformComponent;
class BoxColliderComponent;

class CollisionSystem :public System
{
public:
	CollisionSystem(World* world);

	void Initialize();
	void Update(float deltaTime);

public:

	//void Player2Player(float deltaTime);
	//void Player2Movable(float deltaTime);

	void Movable2Movable(float deltaTime);
	void Movable2Static(float deltaTime);
	
private:
	void AvoidCollisionByMovementState(
		World* world,
		Entity a,
		Entity b,
		BoxColliderComponent* colA,
		BoxColliderComponent* colB,
		float deltaTime);

	
private:
	std::shared_ptr<PhysicsWorld> mPhysicsWorld;
	std::vector<DynamicProxy> mDynamicObjects;
};

//static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);