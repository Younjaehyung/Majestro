#pragma once
#include "UIFeature.h"

class HUDPortraitSlotComponent;

class HUDPortraitUpdateFeature : public UIFeature
{
public:
	void Update(float dt) override;

private:
	static int32 PortraitAtlasRow(uint8 playerType);

	// 초상화 크기와 레이어는 유지하고 캐릭터에 맞는 아틀라스 셀만 교체한다.
	void ApplyTextures(HUDPortraitSlotComponent& slot, uint8 playerType);
	void SetSlotVisible(HUDPortraitSlotComponent& slot, bool visible);

	// HP바
	void UpdateHpBar(HUDPortraitSlotComponent& slot, Entity owner);
	void HideHpBar(HUDPortraitSlotComponent& slot);

private:
	// 채움 텍스처 캔버스 내 실제 바 UV 구간
	Vec2 mHpFillUvRangeX = Vec2(118.f / 768.f, 718.f / 768.f);
};
