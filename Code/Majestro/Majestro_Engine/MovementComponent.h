#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "EnginePch.h"

class MovementComponent : public Component<MovementComponent>
{
public:
	Vec2 mMovingDirection;

	Vec2 mCameraView;

	float mCameraRotationX =0.f;
	float mCameraRotationY =180.f;
};