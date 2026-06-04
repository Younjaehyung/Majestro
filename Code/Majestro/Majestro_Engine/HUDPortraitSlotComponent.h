#pragma once
#include "Component.h"
#include "Entity.h"

// 초상화 컴포넌트
class HUDPortraitSlotComponent : public Component<HUDPortraitSlotComponent>
{
public:
	uint8  mSlotIndex = 0;        // 0 = 메인(로컬 플레이어) 나머지 = 타플레이어

	Entity mBack0 = NULL_ENTITY;  // 배경 0 (layer 1)
	Entity mBack1 = NULL_ENTITY;  // 배경 1 (layer 2)
	Entity mHead0 = NULL_ENTITY;  // 머리 0 (layer 3)
	Entity mHead1 = NULL_ENTITY;  // 머리 1 (layer 4, Bounce 애니메이션)

	// 타 플레이어용 슬롯
	Entity mHpBack = NULL_ENTITY; // 빈 바 배경
	Entity mHpFill = NULL_ENTITY; // 체력바 채움
	Entity mHpText = NULL_ENTITY; // HP 텍스트

	Entity mOwnerEntity = NULL_ENTITY; // 현재 이 슬롯에 배정된 플레이어 엔티티 (HP 조회용)
	uint8  mCurrentType = 0xFF;   // 현재 표시 중인 playerType (0xFF = 빈칸).
	bool   mOccupied = false;     // 현재 슬롯이 플레이어로 채워졌는지
};
