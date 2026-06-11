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
	Vec2 GetProgressAnchorPx() const;
	Vec2 GetProgressSizePx(const Vec2& sizeRatio) const;
	void DrawDebugPanel();

private:
	// 페이즈 진행 UI 앵커.
	// 위치와 크기는 비율값만 저장하고, 실제 픽셀값은 RENDERMANAGER.GetWindow()의 현재 해상도로 계산
	Vec2  mProgressAnchorRatio = Vec2(0.5f, 0.1019f);

	// Conquest UI.
	Vec2  mConquestSizeRatio = Vec2(0.2667f, 0.4741f);
	float mConquestInnerRadius = 0.78f;            // 도넛 두께 (0=원판, 1=링 없음)
	std::wstring mConquestBgTextureName = L"UI_Ingame_Conquest_Info_0";
	std::wstring mConquestFillTextureName = L"UI_Ingame_Conquest_Info_1";

	float mConquestProgress = 0.f;
	int32 mCachedConquestWaveCheckPoint = 0;

	// Escort UI.
	Vec2  mEscortSizeRatio = Vec2(0.4f, 0.1185f);
	Vec2  mEscortCursorSizeRatio = Vec2(0.0167f, 0.0296f);

	float mEscortProgress = 0.f;

	// 라인/체크 텍스처 캔버스 안에서 실제 트랙이 차지하는 가로 UV 구간
	// UI_Escort_Info_1 (1536px):  픽셀 195~1385 이면 195/1536, 1385/1536
	Vec2  mEscortFillUvRange = Vec2(195.f / 1536.f, 1385.f / 1536.f);

	std::wstring mEscortBgTextureName = L"UI_Escort_Info_0";
	std::wstring mEscortLineTextureName = L"UI_Escort_Info_1";
	std::wstring mEscortCheckTextureName = L"UI_Escort_Info_2";
	std::wstring mEscortCursorTextureName = L"UI_Escort_Info_Cursor";
};
