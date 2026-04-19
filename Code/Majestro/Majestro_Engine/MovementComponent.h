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

	bool mAttack = false;
	bool mSkill1 = false;
	bool mSkill2 = false;
	bool mSpecial = false;
	bool mReload = false;


	Vec2 mCameraView;

	float mCameraRotationX =0.f;
	float mCameraRotationY =180.f;

	Vec2 mCurrentRotation = Vec2(0.f,0.f);
	Vec2 mTargetRotation = Vec2(0.f,0.f);



	// Turn In Place
	bool mIsTurnInPlace = false;
	int mTurnInPlaceDir = 0; // -1: left, 1: right, 0: none
	float mTurnStartYaw = 0.f;
	float mTurnTargetYaw = 0.f;

	float mTurnElapsed = 0.f;
	float mTurnDuration = 0.25f;


};

class EnemyMovementComponent : public Component<EnemyMovementComponent>
{
public:
	Vec3 mMovingDirection;
	bool mJump = false;
};
