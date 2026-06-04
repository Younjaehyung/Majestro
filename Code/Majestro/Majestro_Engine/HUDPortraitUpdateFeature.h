#pragma once
#include "UIFeature.h"

class HUDPortraitSlotComponent;

class HUDPortraitUpdateFeature : public UIFeature
{
public:
	void Update(float dt) override;

private:
	static const wchar_t* PortraitPrefix(uint8 playerType);

	// 초상화
	void ApplyTextures(HUDPortraitSlotComponent& slot, uint8 playerType);
	void SetSlotVisible(HUDPortraitSlotComponent& slot, bool visible);

	// HP바
	void UpdateHpBar(HUDPortraitSlotComponent& slot, Entity owner);
	void HideHpBar(HUDPortraitSlotComponent& slot);
};
