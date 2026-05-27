#pragma once
#include "System.h"
#include "UITransformComponent.h"

class LobbyRoomListComponent;

// 메인메뉴 RoomList 상태에서 표시되는 룸 브라우저 시스템
class MenuRoomBrowserSystem : public System
{
public:
    MenuRoomBrowserSystem(World* world);

    void Initialize();
    void Update(float dt);

private:
    void BuildUI();
    void Refresh(LobbyRoomListComponent* list);
    void SetBrowserVisible(bool visible);

    LobbyRoomListComponent* GetList();
    bool IsRoomListState();   // MainMenuController.mState == RoomList

    Entity CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                      const std::wstring& text, const Vec4& color);
    void   SetText(Entity e, const std::wstring& text);
    void   SetVisible(Entity e, bool visible);
    void   SetButtonEnabled(Entity e, bool enabled);

private:
    static constexpr uint8 kRowCount = 6;

    Entity mTitle{};
    Entity mInfoText{};                          // "CONNECTING..." / "NO ROOMS - CREATE ONE"
    Entity mCreateButton{};
    Entity mRefreshButton{};
    std::array<Entity, kRowCount> mRowButtons{};
    std::array<uint32, kRowCount> mRowRoomId{};  // 각 행이 현재 가리키는 roomId (0 = 비어있음)

    bool mUiBuilt = false;
    bool mWasInState = false;        // 직전 프레임에 RoomList 상태였는가
    bool mTransitionRequested = false;
};
