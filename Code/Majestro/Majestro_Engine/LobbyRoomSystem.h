#pragma once
#include "System.h"
#include "UITransformComponent.h"   // Anchor

class LobbyRoomStateComponent;
class LobbyRoomListComponent;
class UIInstanceData;

class LobbyRoomSystem : public System
{
public:
    LobbyRoomSystem(World* world);

    void Initialize();
    void Update(float dt);

private:
    // UI 1회 생성 / 매 프레임 갱신
    void BuildUI();
    void RefreshUI(LobbyRoomStateComponent* state);
    LobbyRoomStateComponent* GetState();
    LobbyRoomListComponent*  GetListState();

    // 대기실(스프라이트) UI 일괄 표시/숨김.
    void SetWaitingRoomVisible(bool visible);

    // 엔티티 생성 헬퍼
    Entity CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                      const std::wstring& text, const Vec4& color);
    Entity CreateCardBg(const Vec2& pos, const Vec2& size, const Vec4& tint);

    // 갱신 헬퍼
    void SetText(Entity e, const std::wstring& text);
    void SetTextColor(Entity e, const Vec4& color);
    void SetVisible(Entity e, bool visible);

    // 이름 변환
    const char*    GetPlayerTypeName(uint8 playerType) const;
    const char*    GetRoomErrorName(uint8 code) const;
    const wchar_t* GetPlayerTypeNameW(uint8 playerType) const;
    const wchar_t* GetRoomErrorNameW(uint8 code) const;

private:
    // UI 엔티티
    static constexpr uint8 kLobbyUiSlotCount = 3;

    struct SlotCardUI
    {
        Entity bg{};
        Entity nameText{};
        Entity charText{};
        Entity readyText{};
        Entity hostText{};
        Entity youText{};
    };

    Entity mHeaderText{};
    Entity mStatusText{};
    std::array<SlotCardUI, kLobbyUiSlotCount> mSlotCards{};
    Entity mReadyButton{};
    Entity mStartButton{};
    Entity mStartReasonText{};
    Entity mErrorToastText{};

    bool mUiBuilt = false;
    bool mStarting = false;       // Start 요청을 보낸 뒤 STARTING 표시용
    bool mRequestedInitialList = false;  // 로비 (재)진입 시 방 목록 1회 요청 여부

    uint8 mLastErrorCode = 0;     // 0 = 없음
    float mErrorRemain = 0.f;

public:
    // 런타임 조정 가능한 색상 팔레트 (Vec4 RGBA)
    Vec4 mColWhite    = { 0.95f, 0.95f, 0.95f, 1.0f };
    Vec4 mColDim      = { 0.45f, 0.45f, 0.50f, 0.8f };
    Vec4 mColYou      = { 1.00f, 0.90f, 0.40f, 1.0f };  // 본인 강조(금색)
    Vec4 mColHost     = { 1.00f, 0.84f, 0.25f, 1.0f };  // Host(금색)
    Vec4 mColReady    = { 0.40f, 1.00f, 0.55f, 1.0f };  // Ready(초록)
    Vec4 mColNotReady = { 0.95f, 0.45f, 0.45f, 1.0f };  // Not Ready(적색)

    Vec4 mBgMe        = { 0.30f, 0.26f, 0.10f, 0.70f };  // 본인 슬롯 배경
    Vec4 mBgOther     = { 0.10f, 0.12f, 0.20f, 0.60f };  // 타인 슬롯 배경
    Vec4 mBgEmpty     = { 0.04f, 0.04f, 0.06f, 0.40f };  // 빈 슬롯 배경

    Vec4 mBtnNormal   = { 0.18f, 0.22f, 0.55f, 0.92f };
    Vec4 mBtnHovered  = { 0.30f, 0.38f, 0.85f, 1.00f };
    Vec4 mBtnPressed  = { 0.10f, 0.13f, 0.35f, 1.00f };
    Vec4 mBtnDisabled = { 0.22f, 0.22f, 0.26f, 0.70f };  // Start 비활성
};
