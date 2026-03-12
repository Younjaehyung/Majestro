#pragma once
#include "System.h"
#include "GameEvents.h"

class MeleeAttackSystem : public System
{
public:
	MeleeAttackSystem(World* world);
	void Update(float dt) override;

private:
	void ProcessMeleeAttack(const EvMeleeAttackRequest& request);
};