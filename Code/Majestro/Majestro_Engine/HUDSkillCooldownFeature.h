#pragma once
#include "UIFeature.h"

class HUDSkillCooldownFeature : public UIFeature
{
public:
	void Update(float dt) override;

private:
	// 슬롯별 직전 프레임 쿨타임 진행 여부 (skill1, skill2, attack0, reload)
	bool mWasCooling[4] = { false, false, false, false };
};
