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
	void DrawEscortBar();
private:
	// Conquest UI
	// 1920x1080 기준 화면 상단 중앙
	Vec2  mConquestScreenAnchorPx = Vec2(960.f, 110.f);
	Vec2  mConquestSizePx         = Vec2(180.f, 180.f);
	float mConquestInnerRadius    = 0.78f;            // 도넛 두께 (0=원판, 1=링 없음)
	std::wstring mConquestBgTextureName   = L"UI_Ingame_Conquest_Info_0";
	std::wstring mConquestFillTextureName = L"UI_Ingame_Conquest_Info_1";


	float mConquestProgress = 0.f;
	int32 mCachedConquestWaveCheckPoint = 0;

	// Escort UI
	// 1920x1080 기준 화면 상단 중앙 — bar 의 정중앙이 앵커 위치에 오도록 그려짐
	Vec2  mEscortAnchorPx     = Vec2(960.f, 110.f);
	Vec2  mEscortSizePx       = Vec2(600.f,  40.f); // BG / Line / Check 공통 크기
	Vec2  mEscortCursorSizePx = Vec2( 48.f,  64.f); // 화물(커서) 스프라이트 크기

	float mEscortProgress = 0.f;
	std::wstring mEscortBgTextureName = L"UI_Escort_Info_0";
	std::wstring mEscortLineTextureName = L"UI_Escort_Info_1";
	std::wstring mEscortCheckTextureName = L"UI_Escort_Info_2";
	std::wstring mEscortCursorTextureName = L"UI_Escort_Info_Cursor";

};
