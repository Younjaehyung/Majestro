#pragma once
#include "System.h"

// 광장 NPC 상호작용
class NpcInteractionSystem : public System
{
public:
	NpcInteractionSystem(World* world);

	virtual void Update(float deltaTime) override;
};
