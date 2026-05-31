#pragma once
#include "Component.h"
#include "Entity.h"

class GravityComponent : public Component<GravityComponent>
{
public:
	float mGravity = 0.0f;
	float mGravityA = 9.8f * 100.0f; //중력가속도
	float mHight = 0.0f; //플레이어 높이
	float mGround = 0.0f;
	bool mFalling = false;		// 지금 공중에 있나
	bool mDropping = false;		// 단차에서 떨어지기로 판정한 낙하

	float mHeightInterpolation = 3.0f;

	float mStepDownDistance = 60.0f;
};