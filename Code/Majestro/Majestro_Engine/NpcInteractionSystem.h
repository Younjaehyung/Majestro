#pragma once
#include "System.h"

// 광장 NPC 상호작용
class NpcInteractionSystem : public System
{
public:
	NpcInteractionSystem(World* world);

	virtual void Update(float deltaTime) override;

private:
	// 선택 UI 열림/닫힘
	bool mPrevSelectUiActive = false;
	bool mSavedMouseLook = true;
};
