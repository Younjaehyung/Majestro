#pragma once
#include "Component.h"
#include "TransformComponent.h"

static constexpr int ENEMY_MAX_WAYPOINTS = 256;

class PlayerMovementComponent : public Component<PlayerMovementComponent>
{
public:
	Vec3 mMovingDirection;

	bool mJump = false;

	Vec2 mCameraView;

	float mCameraRotationX = 0.f;
	float mCameraRotationY = 180.f;

	Vec3 mNavPosition;
	bool mNavPositionValid = false;
};

class EnemyMovementComponent : public Component<EnemyMovementComponent>
{
public:
	Vec3 mMovingDirection;
	float mMovingSpeed = 50.0f;
	bool mJump = false;

	// ---- 경로 추적 상태 ----
	Vec3  mPath[ENEMY_MAX_WAYPOINTS];   // 힙 할당 없는 고정 웨이포인트 배열
	int   mPathCount = 0;             // 유효 웨이포인트 수
	int   mPathIndex = 0;             // 현재 이동 목표 인덱스

	// ---- 재탐색 제어 ----
	Vec3  mTarget;                       // 마지막 탐색 목적지
	float mPathTimer = 0.f;           // 남은 쿨다운
	float mPathInterval = 0.5f;          // 재탐색 주기 (초)

	// ---- 정체 감지 / detour 복구 ----
	Vec3  mProgressSamplePos = Vec3::Zero;
	float mProgressSampleElapsed = 0.f;
	float mStuckElapsed = 0.f;

	bool  mDetourActive = false;
	Vec3  mDetourStartPos = Vec3::Zero;
	float mDetourEndTime = 0.f;
};
