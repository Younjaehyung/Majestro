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

	// 수평 이동을 Jolt StaticCollision(벽+바닥) 구 sweep 으로 해석하고 벽을 따라 슬라이드
	Vec3 ResolvePlayerMoveByJolt(GravityComponent* grav, const Vec3& prevPos, const Vec3& desiredEnd);
	Vec3 SweepSlideHorizontal(GravityComponent* grav, const Vec3& prevPos, const Vec3& desiredEnd);

private:
	float WrapAngleDeg(float angleDeg);
	float MoveTowardsAngleDeg(float currentDeg, float targetDeg, float maxDeltaDeg);
};

