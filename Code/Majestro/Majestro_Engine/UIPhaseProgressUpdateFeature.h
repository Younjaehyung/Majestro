#pragma once
#include "UIComponent.h"
#include "UIFeature.h"

class GameConquestComponent;
class GameEscortComponent;

class UIPhaseProgressUpdateFeature : public UIFeature
{
public:
	void Initialize(World* world);
	void Update(float dt);
	void WorldRender(CameraComponent* camera);
	void SpriteRender(DirectX::SpriteBatch* spriteBatch);
	void CustomSpriteRender(std::vector<UIInstanceData>& instances);
	void PostSpriteRender(std::vector<UIInstanceData>& instances);

private:
	void UpdateConquestProgress(float dt, GameConquestComponent* conquestComp);
	void UpdateEscortProgress(float dt, GameEscortComponent* escortComp);

	void DrawConquestRing();

private:
	// 1920x1080 기준 화면 상단 중앙
	Vec2  mConquestScreenAnchorPx = Vec2(960.f, 110.f);
	Vec2  mConquestSizePx         = Vec2(180.f, 180.f);
	float mConquestInnerRadius    = 0.78f;            // 도넛 두께 (0=원판, 1=링 없음)
	std::wstring mConquestBgTextureName   = L"UI_Ingame_Conquest_Info_0";
	std::wstring mConquestFillTextureName = L"UI_Ingame_Conquest_Info_1";


	float mCachedConquestProgress      = -1.f; // -1 = Conquest phase 가 아니므로 그리지 않음
	int32 mCachedConquestWaveCheckPoint = 0;
};
