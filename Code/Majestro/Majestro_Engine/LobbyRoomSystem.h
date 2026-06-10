#pragma once
#include "System.h"
#include "UITransformComponent.h"

class LobbyRoomStateComponent;
class LobbyRoomListComponent;

class LobbyRoomSystem : public System
{
public:
    LobbyRoomSystem(World* world);

    void Initialize();
    void Update(float dt);
public:
    // ImGui 디버그
    // 캐릭터의 info/ready/choose 오버레이 엔티티를 노출
    void GetOverlayEntities(uint8 idx, Entity& info, Entity& ready,
                            std::array<Entity, 3>& choose,
                            std::array<Entity, 3>& lock) const;
    static constexpr uint8 DebugCharacterCount() { return kLobbyCharacterCount; }
    // 켜면 RefreshCharacterOverlay 가 스냅샷과 무관하게 ready/choose 를 모두 보이게 한다.
    bool mDebugForceShowOverlays = false;

private:
    void BuildUI();
    void RefreshUI(LobbyRoomStateComponent* state);
    LobbyRoomStateComponent* GetState();
    LobbyRoomListComponent*  GetListState();

    void SetWaitingRoomVisible(bool visible);
    void RefreshCharacterOverlay(LobbyRoomStateComponent* state, bool hasSnap, uint8 count, uint32 myClientId);

    Entity CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                      const std::wstring& text, const Vec4& color);
    // 아틀라스 시트용
    Entity CreateAtlasSprite(const Vec2& pos, const Vec2& size, Anchor anchor,
                             const RECT& srcRect, uint8 layer, Vec4 tint = { 1.f, 1.f, 1.f, 1.f });

    void SetText(Entity e, const std::wstring& text);
    void SetTextColor(Entity e, const Vec4& color);
    void SetVisible(Entity e, bool visible);

private:
   
    std::wstring mSheetName = L"UI_Robby_Sheet";

    static constexpr uint8 kLobbyUiSlotCount = 3;
    static constexpr uint8 kLobbyPlayerCount = 3;
    static constexpr uint8 kLobbyCharacterCount = 3;

    struct SlotCardUI
    {
        Entity bg{};
        Entity nameText{};
        Entity charText{};
        Entity readyText{};
        Entity hostText{};
        Entity youText{};
    };

    struct CharacterOverlayUI
    {
        Entity info{};
        Entity ready{};
        std::array<Entity, kLobbyPlayerCount> choose{};   // 선택 토큰
        std::array<Entity, kLobbyPlayerCount> lock{};     // 잠금 마커
    };

    Entity mHeaderText{};
    Entity mStatusText{};
    std::array<SlotCardUI, kLobbyUiSlotCount> mSlotCards{};
    std::array<CharacterOverlayUI, kLobbyCharacterCount> mCharacterOverlays{};
    Entity mReadyButton{};
    Entity mStartButton{};
    Entity mExitButton{};
    Entity mStartReasonText{};
    Entity mErrorToastText{};

    bool mUiBuilt = false;
    bool mStarting = false;
    bool mLeaving = false;          // 방 나가기
    bool mWasInRoom = false;
    bool mRequestedInitialList = false;

    uint8 mLastErrorCode = 0;
    float mErrorRemain = 0.f;

public:
    Vec4 mColWhite    = { 0.95f, 0.95f, 0.95f, 1.0f };
    Vec4 mColDim      = { 0.45f, 0.45f, 0.50f, 0.8f };
    Vec4 mColYou      = { 1.00f, 0.90f, 0.40f, 1.0f };
    Vec4 mColHost     = { 1.00f, 0.84f, 0.25f, 1.0f };
    Vec4 mColReady    = { 0.40f, 1.00f, 0.55f, 1.0f };
    Vec4 mColNotReady = { 0.95f, 0.45f, 0.45f, 1.0f };

    Vec4 mBgMe        = { 0.30f, 0.26f, 0.10f, 0.70f };
    Vec4 mBgOther     = { 0.10f, 0.12f, 0.20f, 0.60f };
    Vec4 mBgEmpty     = { 0.04f, 0.04f, 0.06f, 0.40f };

    Vec4 mBtnNormal   = { 0.18f, 0.22f, 0.55f, 0.92f };
    Vec4 mBtnHovered  = { 0.30f, 0.38f, 0.85f, 1.00f };
    Vec4 mBtnPressed  = { 0.10f, 0.13f, 0.35f, 1.00f };
    Vec4 mBtnDisabled = { 0.22f, 0.22f, 0.26f, 0.70f };
};
