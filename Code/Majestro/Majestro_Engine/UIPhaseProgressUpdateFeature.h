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
	void PostSpriteRender(std::vector<UIInstanceData>& instances);

private:
	void UpdateConquestProgress(float dt, GameConquestComponent* conquestComp);
	void UpdateEscortProgress(float dt, GameEscortComponent* escortComp);

	// 점령 UI
	void DrawConquestBackground();
	void DrawConquestRoulette();

	// slotOffset: 0=현재 점령지(호의 정점), +1=다음(우측), -1=점령 완료(좌측).
	void DrawConquestSlot(float slotOffset, float progress, float alpha, bool glow, float extraScale = 1.f);

	void DrawEscortBar();

	Vec2 GetProgressAnchorPx() const;
	Vec2 GetProgressSizePx(const Vec2& sizeRatio) const;

	Vec2 GetConquestBackAnchorPx() const;
	Vec2 GetConquestBackSizePx() const;
	void GetConquestArcGeometry(Vec2& outCenterPx, Vec2& outRadiusPx) const;


	Vec2 GetConquestSlotPosPx(float slotOffset) const;
	float GetConquestSlotScale(float slotOffset) const;
	float GetConquestSlotBaseDiaPx() const;

	void DrawDebugPanel();

private:
	// 페이즈 진행 UI 앵커.
	Vec2  mProgressAnchorRatio = Vec2(0.5f, 0.1019f);

	// --- Conquest 공통 ---
	float mConquestInnerRadius = 0.0f;            // 도넛 두께 (0=원판, 1=링 없음)
	std::wstring mConquestBgTextureName = L"UI_Ingame_Conquest_Info_0";
	std::wstring mConquestFillTextureName = L"UI_Ingame_Conquest_Info_1";

	float mConquestProgress = 0.f;

	// --- 배경 레이어 ---
	std::wstring mConquestBackTextureName = L"UI_Ingame_Conquest_Back";
	Vec2  mConquestBackAnchorRatio = Vec2(0.5f, 0.0889f);
	float mConquestBackHeightRatio = 0.17775f;  // 배너 높이 / 화면 높이

	// --- 룰렛 ---
	float mConquestArcStepDegrees = 33.f;		// 슬롯 간격 각도
	float mConquestSlotDiaOverRadius = 0.62f;	// 슬롯 직경 / 도넛 반지름
	float mConquestSideScale = 0.40f;		// 슬롯 측면 축소 비율 (0=측면 안보임, 1=측면 그대로)
	float mConquestSlotFadeSpan = 2.0f;		// 슬롯 투명화 범위 (슬롯 간격 수)

	// 룰렛 회전 + 완료 플래시 상태
	int32 mConquestZoneCount   = 3;    // 총 점령지 수 (1이면 정점 슬롯만 표시)
	int32 mCurrentZoneIdx      = 0;    // 현재 점령 중인 슬롯
	float mRollT               = 1.f;  // 0=회전 시작, 1=정착
	float mRollDuration        = 0.45f;// 한 칸 회전 시간 (초)
	int32 mCompletedSlotIdx    = -1;   // 방금 점령 완료된 슬롯(플래시 대상), -1=없음

	// [디버그] 켜면 서버 값으로 슬롯을 덮어쓰지 않아 ImGui로 회전 연출 확인 가능
	bool  mDebugDriveRoulette  = false;

	// --- Escort UI ---
	Vec2  mEscortSizeRatio = Vec2(0.4f, 0.1185f);
	Vec2  mEscortCursorSizeRatio = Vec2(0.0167f, 0.0296f);

	float mEscortProgress = 0.f;


	Vec2  mEscortFillUvRange = Vec2(195.f / 1536.f, 1385.f / 1536.f);

	std::wstring mEscortBgTextureName = L"UI_Escort_Info_0";
	std::wstring mEscortLineTextureName = L"UI_Escort_Info_1";
	std::wstring mEscortCheckTextureName = L"UI_Escort_Info_2";
	std::wstring mEscortCursorTextureName = L"UI_Escort_Info_Cursor";
};
