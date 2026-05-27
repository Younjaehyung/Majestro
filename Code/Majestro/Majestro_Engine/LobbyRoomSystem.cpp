#include "pch.h"
#include "LobbyRoomSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "PlayerComponent.h"
#include "Texture.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIButtonComponent.h"
#include "UIButtonFactory.h"

namespace
{
    // 슬롯 카드 가로 배치 (ref 2560x1440)
    constexpr float kCardX[3] = { -470.f, 0.f, 470.f };
}

LobbyRoomSystem::LobbyRoomSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Sim;
}


void LobbyRoomSystem::Initialize()
{
    BuildUI();
}


void LobbyRoomSystem::BuildUI()
{
    // 상단 헤더
    mHeaderText = CreateText({ 0.f, 90.f },  { 1000.f, 90.f }, Anchor::TopCenter, L"ROOM --", mColWhite);
    mStatusText = CreateText({ 0.f, 180.f }, { 1000.f, 70.f }, Anchor::TopCenter, L"WAITING FOR ROOM STATE", mColDim);

    // 하단 슬롯 카드 3개
    for (uint8 i = 0; i < kLobbyUiSlotCount; ++i)
    {
        const float x = kCardX[i];
        SlotCardUI& c = mSlotCards[i];

        c.bg        = CreateCardBg({ x, -300.f }, { 380.f, 470.f }, mBgEmpty);

        std::wstring name = L"PLAYER " + std::to_wstring(static_cast<int>(i) + 1);
        c.nameText  = CreateText({ x, -500.f }, { 320.f, 60.f }, Anchor::BottomCenter, name,          mColDim);
        c.hostText  = CreateText({ x, -440.f }, { 320.f, 50.f }, Anchor::BottomCenter, L"HOST",        mColHost);
        c.youText   = CreateText({ x, -395.f }, { 320.f, 50.f }, Anchor::BottomCenter, L"YOU",         mColYou);
        c.charText  = CreateText({ x, -300.f }, { 320.f, 60.f }, Anchor::BottomCenter, L"",            mColWhite);
        c.readyText = CreateText({ x, -150.f }, { 320.f, 60.f }, Anchor::BottomCenter, L"EMPTY",       mColDim);

        SetVisible(c.hostText, false);
        SetVisible(c.youText, false);
    }

    // 우하단 액션 버튼
    const Vec2 btnSize = { 360.f, 90.f };

    mReadyButton = CreateUIButton(mWorld, {
        .anchor       = Anchor::BottomRight,
        .position     = Vec2(-230.f, -240.f),
        .size         = btnSize,
        .visual       = UIButtonVisual::Texture,
        .resKey       = L"UI_Loading_Main_01",
        .label        = L"READY",
        .normalColor  = mBtnNormal,
        .hoveredColor = mBtnHovered,
        .pressedColor = mBtnPressed,
        .onClick      = [this]()
        {
            // 클릭 시점의 본인 ready 값을 다시 읽어 반전 후 전송
            LobbyRoomStateComponent* st = GetState();
            bool cur = false;
            if (st)
            {
                const uint32 me = Network::GetInstance().mClientId;
                for (const auto& s : st->mSlots)
                    if (s.sessionId != 0 && s.sessionId == me) { cur = s.ready; break; }
            }
            if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomReadyChanged{ !cur });
        },
    });

    mStartButton = CreateUIButton(mWorld, {
        .anchor       = Anchor::BottomRight,
        .position     = Vec2(-230.f, -120.f),
        .size         = btnSize,
        .visual       = UIButtonVisual::Texture,
        .resKey       = L"UI_Loading_Main_01",
        .label        = L"START",
        .normalColor  = mBtnNormal,
        .hoveredColor = mBtnHovered,
        .pressedColor = mBtnPressed,
        .onClick      = [this]()
        {
            LobbyRoomStateComponent* st = GetState();
            if (!st || !st->mHasSnapshot) return;

            const uint32 me = Network::GetInstance().mClientId;
            const uint8 cnt = (st->mPlayerCount < ROOM_MAX_PLAYERS) ? st->mPlayerCount : ROOM_MAX_PLAYERS;
            bool host = false;
            bool all  = (cnt > 0);
            for (uint8 i = 0; i < cnt; ++i)
            {
                const auto& s = st->mSlots[i];
                if (s.sessionId == me && s.isHost) host = true;
                if (!s.ready) all = false;
            }
            if (host && all)
            {
                if (auto em = mWorld->GetEventManager())
                    em->Enqueue(EvNetSceneChange{ SceneId::FirstGame });
                mStarting = true;
            }
        },
    });

    // 버튼 배경이 첫 프레임부터 tint 되도록 초기화
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mReadyButton)) sp->mColorTint = mBtnNormal;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mStartButton)) sp->mColorTint = mBtnNormal;

    // 시작 불가 이유 / 에러
    mStartReasonText = CreateText({ 0.f, -60.f }, { 900.f, 60.f }, Anchor::BottomCenter, L"", mColNotReady);
    mErrorToastText  = CreateText({ 0.f, 350.f }, { 1200.f, 80.f }, Anchor::Center,      L"", mColNotReady);
    SetVisible(mStartReasonText, false);
    SetVisible(mErrorToastText, false);

    mUiBuilt = true;
}

void LobbyRoomSystem::Update(float dt)
{
    // RoomError
    if (auto eventMgr = mWorld->GetEventManager())
    {
        eventMgr->Consume<EvRoomError>([this](const EvRoomError& e)
        {
            mLastErrorCode = e.errorCode;
            mErrorRemain = 3.0f;
        });
    }

    if (mErrorRemain > 0.f)
    {
        mErrorRemain -= dt;
        if (mErrorRemain <= 0.f) mLastErrorCode = 0;
    }

    if (!mUiBuilt) return;

    // 로비 (재)진입 시  방 목록 요청.  
    if (!mRequestedInitialList && GetListState() != nullptr)
    {
        if (auto em = mWorld->GetEventManager())
            em->Enqueue(EvRoomListRequest{});
        mRequestedInitialList = true;
    }

    RefreshUI(GetState());
}

void LobbyRoomSystem::RefreshUI(LobbyRoomStateComponent* state)
{
    // 에러
    {
        const bool showToast = (mLastErrorCode != 0) && (mErrorRemain > 0.f);
        SetVisible(mErrorToastText, showToast);
        if (showToast) SetText(mErrorToastText, GetRoomErrorNameW(mLastErrorCode));
    }

    // 방 목록창 이면 대기실 스프라이트 UI 를 끄기
    LobbyRoomListComponent* listState = GetListState();
    const bool inRoom = (listState != nullptr) && (listState->mCurrentRoomId != 0);
    SetWaitingRoomVisible(inRoom);
    if (!inRoom)
        return;

    const uint32 myClientId = Network::GetInstance().mClientId;
    const bool hasSnap = (state != nullptr) && state->mHasSnapshot;
    const uint8 count  = state ? ((state->mPlayerCount < kLobbyUiSlotCount) ? state->mPlayerCount : kLobbyUiSlotCount) : 0;

    bool isMeHost = false;
    bool myReady  = false;
    bool allReady = hasSnap && (count > 0);

    // 슬롯 카드 갱신
    for (uint8 i = 0; i < kLobbyUiSlotCount; ++i)
    {
        SlotCardUI& c = mSlotCards[i];
        const bool occupied = hasSnap && (i < count) && (state->mSlots[i].sessionId != 0);

        if (occupied)
        {
            const auto& slot = state->mSlots[i];
            const bool isMe = (slot.sessionId == myClientId);
            if (isMe) { myReady = slot.ready; if (slot.isHost) isMeHost = true; }
            if (!slot.ready) allReady = false;

            SetTextColor(c.nameText, isMe ? mColYou : mColWhite);
            SetText(c.charText, GetPlayerTypeNameW(slot.playerType));
            SetVisible(c.charText, true);
            SetText(c.readyText, slot.ready ? L"READY" : L"NOT READY");
            SetTextColor(c.readyText, slot.ready ? mColReady : mColNotReady);
            SetVisible(c.hostText, slot.isHost);
            SetVisible(c.youText, isMe);

            if (auto* sp = mWorld->GetComponent<UISpriteComponent>(c.bg))
                sp->mColorTint = isMe ? mBgMe : mBgOther;
        }
        else
        {
            SetTextColor(c.nameText, mColDim);
            SetVisible(c.charText, false);
            SetText(c.readyText, L"EMPTY");
            SetTextColor(c.readyText, mColDim);
            SetVisible(c.hostText, false);
            SetVisible(c.youText, false);

            if (auto* sp = mWorld->GetComponent<UISpriteComponent>(c.bg))
                sp->mColorTint = mBgEmpty;
        }
    }

    // 헤더 / 상태 문구
    {
        const unsigned maxDisplay =
            (state && state->mMaxPlayers >= 1 && state->mMaxPlayers <= ROOM_MAX_PLAYERS)
            ? state->mMaxPlayers : ROOM_MAX_PLAYERS;

        wchar_t buf[96];
        swprintf_s(buf, L"ROOM %u    %u / %u",
            state ? state->mRoomId : 0u,
            static_cast<unsigned>(state ? state->mPlayerCount : 0),
            maxDisplay);
        SetText(mHeaderText, buf);

        if (!hasSnap)        { SetText(mStatusText, L"WAITING FOR ROOM STATE"); SetTextColor(mStatusText, mColDim); }
        else if (mStarting)  { SetText(mStatusText, L"STARTING");               SetTextColor(mStatusText, mColYou); }
        else if (allReady)   { SetText(mStatusText, L"READY TO START");         SetTextColor(mStatusText, mColReady); }
        else                 { SetText(mStatusText, L"WAITING");                SetTextColor(mStatusText, mColWhite); }
    }

    // Ready 버튼 (항상 활성)
    SetText(mReadyButton, myReady ? L"CANCEL READY" : L"READY");
    SetTextColor(mReadyButton, mColWhite);

    if (auto* btn = mWorld->GetComponent<UIButtonComponent>(mReadyButton))
    {
        btn->mNormalColor  = mBtnNormal;
        btn->mHoveredColor = mBtnHovered;
        btn->mPressedColor = mBtnPressed;
        if (!btn->mHovered && !btn->mPressed)
            if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mReadyButton))
                sp->mColorTint = mBtnNormal;
    }

    // Start 버튼 (Host + 전원 Ready 일 때만 활성)
    const bool canStart = isMeHost && allReady && hasSnap;
    if (auto* btn = mWorld->GetComponent<UIButtonComponent>(mStartButton))
        btn->mEnabled = canStart;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mStartButton))
        sp->mColorTint = canStart ? mBtnNormal : mBtnDisabled;
    SetTextColor(mStartButton, canStart ? mColWhite : mColDim);

    // 시작 불가 이유
    const wchar_t* reason = nullptr;
    if (!hasSnap)        reason = L"NEED ROOM STATE";
    else if (!isMeHost)  reason = L"HOST ONLY";
    else if (!allReady)  reason = L"WAITING FOR READY";
    SetVisible(mStartReasonText, reason != nullptr);
    if (reason) SetText(mStartReasonText, reason);
}

LobbyRoomStateComponent* LobbyRoomSystem::GetState()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomStateComponent>())
        return nullptr;
    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
    if (entities.empty()) return nullptr;
    return mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]);
}

LobbyRoomListComponent* LobbyRoomSystem::GetListState()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomListComponent>())
        return nullptr;
    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty()) return nullptr;
    return mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
}

void LobbyRoomSystem::SetWaitingRoomVisible(bool visible)
{
    SetVisible(mHeaderText, visible);
    SetVisible(mStatusText, visible);

    for (uint8 i = 0; i < kLobbyUiSlotCount; ++i)
    {
        SlotCardUI& c = mSlotCards[i];
        SetVisible(c.bg, visible);
        SetVisible(c.nameText, visible);
        SetVisible(c.charText, visible);
        SetVisible(c.readyText, visible);
        SetVisible(c.hostText, visible);
        SetVisible(c.youText, visible);
    }

    SetVisible(mReadyButton, visible);
    SetVisible(mStartButton, visible);
    SetVisible(mStartReasonText, visible);

    // 버튼 입력 활성/비활성 (숨김 시 클릭 차단됨).
    if (auto* b = mWorld->GetComponent<UIButtonComponent>(mReadyButton)) b->mEnabled = visible;
    if (auto* b = mWorld->GetComponent<UIButtonComponent>(mStartButton)) b->mEnabled = visible;
}

Entity LobbyRoomSystem::CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                                      const std::wstring& text, const Vec4& color)
{
    Entity e = mWorld->CreateEntity();
    auto& tr = mWorld->AddComponent<UITransformComponent>(e);
    tr.mAnchor       = anchor;
    tr.mPosition     = pos;
    tr.mSize         = size;
    tr.mPivot        = Vec2(0.5f, 0.5f);
    tr.mUILayerIndex = 6;            // 카드 배경(4) 위

    auto& txt = mWorld->AddComponent<UITextComponent>(e);
    txt.mText = text;
    txt.mColor.f[0] = color.x; txt.mColor.f[1] = color.y;
    txt.mColor.f[2] = color.z; txt.mColor.f[3] = color.w;
    return e;
}

Entity LobbyRoomSystem::CreateCardBg(const Vec2& pos, const Vec2& size, const Vec4& tint)
{
    Entity e = mWorld->CreateEntity();
    auto& tr = mWorld->AddComponent<UITransformComponent>(e);
    tr.mAnchor       = Anchor::BottomCenter;
    tr.mPosition     = pos;
    tr.mSize         = size;
    tr.mPivot        = Vec2(0.5f, 0.5f);
    tr.mUILayerIndex = 4;            // 텍스트(6)보다 뒤

    auto& sp = mWorld->AddComponent<UISpriteComponent>(e, RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01"));
    sp.mColorTint = tint;
    return e;
}

void LobbyRoomSystem::SetText(Entity e, const std::wstring& text)
{
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
        tx->mText = text;
}

void LobbyRoomSystem::SetTextColor(Entity e, const Vec4& color)
{
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
    {
        tx->mColor.f[0] = color.x; tx->mColor.f[1] = color.y;
        tx->mColor.f[2] = color.z; tx->mColor.f[3] = color.w;
    }
}

void LobbyRoomSystem::SetVisible(Entity e, bool visible)
{
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = visible;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = visible;
}


const char* LobbyRoomSystem::GetPlayerTypeName(uint8 playerType) const
{
    switch (static_cast<PlayerType>(playerType))
    {
    case PlayerType::Rudwig :  return "Rudwig";
    case PlayerType::Ibanix :  return "Ibanix";
    case PlayerType::Fanthor : return "Fanthor";
    default: return "Unknown";
    }
}

const char* LobbyRoomSystem::GetRoomErrorName(uint8 code) const
{
    switch (static_cast<RoomErrorCode>(code))
    {
    case RoomErrorCode::NotHost:           return "Host 가 아닙니다";
    case RoomErrorCode::NotAllReady:       return "전원 Ready 가 아닙니다";
    case RoomErrorCode::NotEnoughPlayers:  return "최소 인원 부족";
    case RoomErrorCode::InvalidRoom:       return "잘못된 방";
    case RoomErrorCode::RoomFull:          return "방이 가득 찼습니다";
    case RoomErrorCode::AlreadyInRoom:     return "이미 다른 방에 있습니다";
    case RoomErrorCode::RoomInGame:        return "게임 중인 방입니다";
    default:                               return "알 수 없음";
    }
}

const wchar_t* LobbyRoomSystem::GetPlayerTypeNameW(uint8 playerType) const
{
    switch (static_cast<PlayerType>(playerType))
    {
    case PlayerType::Rudwig : return L"RUDWIG";
    case PlayerType::Ibanix : return L"IBANIX";
    case PlayerType::Fanthor : return L"FANTHOR";
    default: return L"UNKNOWN";
    }
}

const wchar_t* LobbyRoomSystem::GetRoomErrorNameW(uint8 code) const
{
    switch (static_cast<RoomErrorCode>(code))
    {
    case RoomErrorCode::NotHost:           return L"HOST 가 아닙니다";
    case RoomErrorCode::NotAllReady:       return L"전원 READY 가 아닙니다";
    case RoomErrorCode::NotEnoughPlayers:  return L"최소 인원 부족";
    case RoomErrorCode::InvalidRoom:       return L"잘못된 방";
    case RoomErrorCode::RoomFull:          return L"방이 가득 찼습니다";
    case RoomErrorCode::AlreadyInRoom:     return L"이미 다른 방에 있습니다";
    case RoomErrorCode::RoomInGame:        return L"게임 중인 방입니다";
    default:                               return L"알 수 없음";
    }
}
