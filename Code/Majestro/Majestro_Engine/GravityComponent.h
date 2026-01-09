#pragma once
#include "Component.h"
#include "Entity.h"

class GravityComponent : public Component<GravityComponent>
{
public:
	float mGravity = 0.0f;
	float mGravityA = 0.98f; //중력가속도
	float mHight = 0.0f; //플레이어 높이
	float mGround = 0.0f;
};