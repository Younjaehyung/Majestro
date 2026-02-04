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
	bool mDash = false;

	Vec2 mCameraView;

	float mCameraRotationX =0.f;
	float mCameraRotationY =180.f;
};

class EnemyMovementComponent : public Component<EnemyMovementComponent>
{
public:
	Vec3 mMovingDirection;
	bool mJump = false;
};