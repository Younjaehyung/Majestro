#include "pch.h"
#include "MenuRoomBrowserSystem.h"
#include "Engine.h"
#include "SceneManager.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "MainMenuController.h"
#include "LobbyRoomListComponent.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIVfxComponent.h"
#include "UIButtonComponent.h"
#include "UIButtonFactory.h"

namespace
{
    // UI Title Search 예시처럼 방 검색 UI 전체를 화면 왼쪽 영역에 배치한다.
    constexpr float kPanelCenterX = -500.f;
    constexpr float kRowStartY = -110.f;
    constexpr float kRowGap = 115.f;

    // UI Title Sheet 02의 첫 번째 상자 스프라이트 영역이다.
    constexpr RECT kRoomBoxSprite = { 0, 0, 768, 256 };
}

MenuRoomBrowserSystem::MenuRoomBrowserSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Sim;
}

void MenuRoomBrowserSystem::Initialize()
{
    BuildUI();
}

void MenuRoomBrowserSystem::BuildUI()
{
    const Vec4 colWhite = { 0.95f, 0.95f, 0.95f, 1.0f };
    const Vec4 colDim   = { 0.55f, 0.55f, 0.62f, 0.9f };
    // 긴 방 정보 문구가 테두리를 벗어나지 않도록 상자 크기를 확대한다.
    const Vec2 roomBoxSize = { 940.f, 104.f };
    const Vec2 actionBoxSize = { 360.f, 88.f };

    // 예시 화면의 왼쪽 검색 영역 안에서 제목과 방 목록을 정렬한다.
    mTitle = CreateText(
        { kPanelCenterX, -390.f },
        { 800.f, 90.f },
        Anchor::Center,
        L"ROOM SEARCH",
        colWhite);
    mInfoText = CreateText(
        { kPanelCenterX, -120.f },
        { 900.f, 70.f },
        Anchor::Center,
        L"CONNECTING",
        colDim);

    // CreateRoom과 Refresh는 방 카드와 같은 상자 스프라이트를 축소해 사용한다.
    mCreateButton = CreateUIButton(mWorld, {
        .anchor   = Anchor::Center,
        .position = Vec2(kPanelCenterX - 220.f, -275.f),
        .size     = actionBoxSize,
        .visual   = UIButtonVisual::Texture,
        .resKey   = L"UI_Title_Sheet_02",
        .sourceRect = kRoomBoxSprite,
        .label    = L"CREATE ROOM",
        .onClick  = [this]()
        {
            if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomCreate{});
        },
    });

    mRefreshButton = CreateUIButton(mWorld, {
        .anchor   = Anchor::Center,
        .position = Vec2(kPanelCenterX + 220.f, -275.f),
        .size     = actionBoxSize,
        .visual   = UIButtonVisual::Texture,
        .resKey   = L"UI_Title_Sheet_02",
        .sourceRect = kRoomBoxSprite,
        .label    = L"REFRESH",
        .onClick  = [this]()
        {
            if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomListRequest{});
        },
    });

    // 각 방은 상자 스프라이트 위에 방 정보를 글꼴로 출력하며 상자 전체를 입장 버튼으로 사용한다.
    for (uint8 i = 0; i < kRowCount; ++i)
    {
        const uint8 idx = i;
        mRowButtons[i] = CreateUIButton(mWorld, {
            .anchor   = Anchor::Center,
            .position = Vec2(
                kPanelCenterX,
                kRowStartY + kRowGap * static_cast<float>(i)),
            .size     = roomBoxSize,
            .visual   = UIButtonVisual::Texture,
            .resKey   = L"UI_Title_Sheet_02",
            .sourceRect = kRoomBoxSprite,
            .label    = L"",
            .onClick  = [this, idx]()
            {
                if (mRowRoomId[idx] != 0)
                    if (auto em = mWorld->GetEventManager())
                        em->Enqueue(EvRoomJoin{ mRowRoomId[idx] });
            },
        });
        mRowRoomId[i] = 0;
    }


    SetBrowserVisible(false);
    mUiBuilt = true;
}

void MenuRoomBrowserSystem::Update(float dt)
{
    if (!mUiBuilt) return;

    const bool inState = IsRoomListState();

    // 상태 진입/이탈 처리
    if (inState != mWasInState)
    {
        SetBrowserVisible(inState);
        if (inState)
        {
           if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomListRequest{});
        }
        mWasInState = inState;
    }

    if (!inState) return;

    LobbyRoomListComponent* list = GetList();
    Refresh(list);

    // 방 입장/생성 성공시 로비 Scene(대기실)으로 전환
    if (!mTransitionRequested && list != nullptr && list->mCurrentRoomId != 0)
    {
        mTransitionRequested = true;
        gEngine->GetSceneManager().RequestScene(SceneId::Lobby);
    }
}

void MenuRoomBrowserSystem::Refresh(LobbyRoomListComponent* list)
{
    const uint8 count = list ? ((list->mCount < kRowCount) ? list->mCount : kRowCount) : 0;

    for (uint8 i = 0; i < kRowCount; ++i)
    {
        if (i < count)
        {
            const LobbyRoomListEntry& e = list->mEntries[i];
            mRowRoomId[i] = e.roomId;

            wchar_t buf[96];
            swprintf_s(buf, L"ROOM %u    PLAYERS %u OF %u    STATUS %s",
                e.roomId,
                static_cast<unsigned>(e.playerCount),
                static_cast<unsigned>(e.maxPlayers),
                e.phase == 1 ? L"INGAME" : L"WAITING");
            SetText(mRowButtons[i], buf);
            SetVisible(mRowButtons[i], true);
            SetButtonEnabled(mRowButtons[i], true);
        }
        else
        {
            mRowRoomId[i] = 0;
            SetVisible(mRowButtons[i], false);
            SetButtonEnabled(mRowButtons[i], false);
        }
    }

    // 안내 문구
    const bool empty = (count == 0);
    SetVisible(mInfoText, empty);
    if (empty)
        SetText(
            mInfoText,
            (list && list->mHasList)
                ? L"NO ROOMS    CREATE A ROOM"
                : L"CONNECTING");
}

void MenuRoomBrowserSystem::SetBrowserVisible(bool visible)
{
    SetVisible(mTitle, visible);
    SetVisible(mCreateButton, visible);
    SetVisible(mRefreshButton, visible);
    SetButtonEnabled(mCreateButton, visible);
    SetButtonEnabled(mRefreshButton, visible);

    if (!visible)
    {
        SetVisible(mInfoText, false);
        for (uint8 i = 0; i < kRowCount; ++i)
        {
            SetVisible(mRowButtons[i], false);
            SetButtonEnabled(mRowButtons[i], false);
        }
    }
}

LobbyRoomListComponent* MenuRoomBrowserSystem::GetList()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomListComponent>())
        return nullptr;
    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty()) return nullptr;
    return mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
}

bool MenuRoomBrowserSystem::IsRoomListState()
{
    if (!mWorld->HasComponentPool<MainMenuController>())
        return false;
    auto entities = mWorld->GetEntitiesWithComponent<MainMenuController>();
    if (entities.empty()) return false;
    auto* ctrl = mWorld->GetComponent<MainMenuController>(entities[0]);
    return ctrl != nullptr && ctrl->mState == MainMenuState::RoomList;
}

Entity MenuRoomBrowserSystem::CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                                         const std::wstring& text, const Vec4& color)
{
    Entity e = mWorld->CreateEntity();
    auto& tr = mWorld->AddComponent<UITransformComponent>(e);
    tr.mAnchor       = anchor;
    tr.mPosition     = pos;
    tr.mSize         = size;
    tr.mPivot        = Vec2(0.5f, 0.5f);
    tr.mUILayerIndex = 6;

    auto& txt = mWorld->AddComponent<UITextComponent>(e);
    txt.mText = text;
    txt.mColor.f[0] = color.x; txt.mColor.f[1] = color.y;
    txt.mColor.f[2] = color.z; txt.mColor.f[3] = color.w;
    return e;
}

void MenuRoomBrowserSystem::SetText(Entity e, const std::wstring& text)
{
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
        tx->mText = text;
}

void MenuRoomBrowserSystem::SetVisible(Entity e, bool visible)
{
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = visible;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = visible;
    if (auto* vf = mWorld->GetComponent<UIVfxComponent>(e))    vf->mVisible = visible;
}

void MenuRoomBrowserSystem::SetButtonEnabled(Entity e, bool enabled)
{
    if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e)) bt->mEnabled = enabled;
}
