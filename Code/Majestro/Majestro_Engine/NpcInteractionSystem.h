#pragma once
#include "System.h"

// 광장 NPC 상호작용
class NpcInteractionSystem : public System
{
public:
	NpcInteractionSystem(World* world);

	virtual void Update(float deltaTime) override;

private:
	// 레벨 선택 UI 열림/닫힘에 따른 마우스룩(커서) 전환 추적
	bool mPrevLevelSelectActive = false;
	bool mSavedMouseLook = true;
};
