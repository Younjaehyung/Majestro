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
	bool mDropping = false;		// 단차에서 떨어지기로 판정한 낙하(정보용)

	float mHeightInterpolation = 3.0f;

	float mStepDownDistance = 60.0f;	// 이 이하 단차는 스냅(계단), 초과는 낙하(단차)

	// 발밑 레이가 일시적으로 빗나가도 직전 바닥을 유지하는 유예 시간.
	float mGroundGraceLeft = 0.0f;

	// 단차 낙하 확정 디바운스.
	// 곧바로 낙하로 확정하지 않고, dist 초과가 mDropConfirmDelay 동안 지속될 때만 낙하
	float mDropConfirmDelay = 0.1f;		// 낙하 확정까지 지속돼야 하는 시간(초)
	float mDropConfirmLeft  = 0.1f;		// 남은 확정 카운트다운
};