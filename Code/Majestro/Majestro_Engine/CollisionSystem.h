#pragma once
#include "World.h"
#include "System.h"
class CollisionSystem :public System
{
public:
	CollisionSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

public:
	DirectX::BoundingOrientedBox mBoundingBoxA;
	DirectX::BoundingOrientedBox mBoundingBoxB;

};

