#pragma once
#include "Component.h"
#include "Entity.h"

class HUDSkillSlotComponent : public Component<HUDSkillSlotComponent>
{
public:
	uint8  mSkillSlot = 0;            // 0=Attack,1=Skill1,2=Skill2,3=Reload
	Entity mKey     = NULL_ENTITY;   // 키(Q/E) 패널
	Entity mBack    = NULL_ENTITY;   // 배경 패널
	Entity mIcon    = NULL_ENTITY;   // 스킬 아이콘
	Entity mOverlay = NULL_ENTITY;   // 쿨타임 오버레이
};
