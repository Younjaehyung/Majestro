#pragma once
#include "World.h"
#include "System.h"


class Navigation;
class NavMesh;


class MovementSystem :public System
{
public:

	MovementSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

private:

	void UpdateEvent(float deltaTime);
	void UpdatePlayer(float deltaTIme);
	void UpdateEnemy(float deltaTime);
	void UpdateGravity(float deltaTime);
	void UpdateBullet(float deltaTime);

private:
	float WrapAngleDeg(float angleDeg);
	float MoveTowardsAngleDeg(float currentDeg, float targetDeg, float maxDeltaDeg);
};

