#pragma once
#include "Component.h"
#include "Entity.h"


class DeathCamComponent : public Component<DeathCamComponent>
{
public:
	// 데스캠 진행 상태
	bool     mActive   = false;        // 데스캠 진행 중
	bool     mWasFall  = false;        // 낙사 여부
	Vec3     mCamPos   = Vec3::Zero;   // 고정된 카메라 위치
	float    mElapsed  = 0.f;          // 사망 후 경과(초)

	// 관전
	bool     mSpectating     = false;  // 관전 진입 여부
	EntityID mSpectateTarget = 0;      // 관전 대상 엔티티(0=없음/자기 시신)
	int      mSpectateCycleReq = 0;    // 관전 순환 입력 의도(+1 다음/-1 이전/0 없음)

	Vec3     mGroundedAnchorPos = Vec3::Zero;	// 낙사 판정 지점
	bool     mHasGroundedAnchor = false;


	float    mSpectateDelay = 5.0f;    // 관전 전환 대기(초)
	float    mFallDeathY    = -200.f;  // 낙사 높이 추정치
	float    mLookHeight    = 90.f;    // 바라볼 지점의 Y 오프셋
};
