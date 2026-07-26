#pragma once
#include "Component.h"
#include "Entity.h"

class HUDSkillSlotComponent : public Component<HUDSkillSlotComponent>
{
public:
	uint8  mSkillSlot = 0;            // 0=Attack,1=Skill1,2=Skill2,3=Reload (255=궁극기: 쿨타임 대신 리듬 충전)
	Entity mKey       = NULL_ENTITY; // 키(E/Shift/Q) 뱃지
	Entity mBack      = NULL_ENTITY; // 배경 패널 (궁극기는 ready 글로우로도 사용)
	Entity mIcon      = NULL_ENTITY; // 스킬 아이콘 (쿨/미충전 시 디새추레이션)
	Entity mOverlay   = NULL_ENTITY; // 쿨타임/충전 덮개 (세로 크롭)

	bool   mIsUltimate = false;      // true면 쿨다운 대신 리듬 충전 게이지로 동작

	// 런타임 — per-slot 연출 상태는 여기(컴포넌트)에 둔다 (피처 측 배열 금지)
	bool   mPrevReady = true;        // 직전 프레임 사용가능 여부 (준비 완료 에지 감지)
	float  mPopTimer  = 0.f;         // 준비 완료 팝 애니메이션 남은시간(초)
};
