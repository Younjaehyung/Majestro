#pragma once
#include "Component.h"
#include "TransformComponent.h"

class MovementComponent : public Component<MovementComponent>
{
public:
	Vec3 mMovingDirection;

	Vec2 mCameraView;

	float mCameraRotationX =0.f;
	float mCameraRotationY =180.f;
};