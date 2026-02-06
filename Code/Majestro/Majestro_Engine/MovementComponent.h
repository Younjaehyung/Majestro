#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "EnginePch.h"

class PlayerMovementComponent : public Component<PlayerMovementComponent>
{
public:
	Vec3 mMovingDirection;
	bool mJump = false;
	bool mDash = false;
	bool mAttack1 = false;


	Vec2 mCameraView;

	float mCameraRotationX =0.f;
	float mCameraRotationY =180.f;

	float currentX = 0.0f;
	float currentY = 0.0f;
	float targetX = 0.0f;
	float targetY = 0.0f;


};

class EnemyMovementComponent : public Component<EnemyMovementComponent>
{
public:
	Vec3 mMovingDirection;
	bool mJump = false;
};