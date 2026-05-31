#pragma once
#include "World.h"
#include "System.h"


class Navigation;
class NavMesh;
class TransformComponent;
class GravityComponent;


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

	// 떨어질 수 있는지 Jolt로 판별후, 가능하면 낙하
	bool TryStartEdgeDrop(Entity entity, TransformComponent* tf, GravityComponent* grav,
		const Vec3& prevPos, const Vec3& desiredEnd);

private:
	float WrapAngleDeg(float angleDeg);
	float MoveTowardsAngleDeg(float currentDeg, float targetDeg, float maxDeltaDeg);
};

