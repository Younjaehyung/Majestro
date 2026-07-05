#include "pch.h"
#include "LobbyRoomSystem.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"
#include "Texture.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIButtonComponent.h"
#include "UIButtonFactory.h"

namespace
{
    constexpr LONG kInfoW   = 384, kInfoH   = 256;
    constexpr LONG kReadyW  = 384, kReadyH  = 128;
    constexpr LONG kChooseW = 128, kChooseH = 128;
    constexpr LONG kLockW = 256, kLockH = 256;


    inline RECT MakeAtlasRect(LONG x, LONG y, LONG w, LONG h)
    {   // 아틀라스 텍스쳐 자르는 용도
        return RECT{ x, y, x + w, y + h };
    }

    struct CharacterUiLayout
    {
        uint8 playerType;

        RECT infoSrc;       // 이름
        RECT readySrc;      // ready to rock
        RECT chooseSrc;     // 사각형

        Vec2 infoPos;
        Vec2 readyPos;
    };

    const CharacterUiLayout kCharacterUiLayouts[] =
    {
        // Rudwig = 노랑(DRUM): Info(768,512) / Ready(0,384) / Choose(128,0)
        { static_cast<uint8>(PlayerType::Rudwig),
          MakeAtlasRect(384, 512, kInfoW, kInfoH) , MakeAtlasRect(384, 384, kReadyW, kReadyH), MakeAtlasRect(128, 0, kChooseW, kChooseH),
          { 850.f,  145.f }, { 580.f, -105.f } },
        // Ibanix = 초록(BASS): Info(0,512) / Ready(384,384) / Choose(256,0)
        { static_cast<uint8>(PlayerType::Ibanix),
          MakeAtlasRect(0,   512, kInfoW, kInfoH), MakeAtlasRect(0,   384, kReadyW, kReadyH), MakeAtlasRect(0,   0, kChooseW, kChooseH),
          { 260.f,  200.f }, {  40.f,   25.f } },
        // Fanthor = 보라(ELEC): Info(384,512) / Ready(768,384) / Choose(0,0)
        { static_cast<uint8>(PlayerType::Fanthor),
         MakeAtlasRect(768, 512, kInfoW, kInfoH), MakeAtlasRect(768, 384, kReadyW, kReadyH), MakeAtlasRect(256, 0, kChooseW, kChooseH),
          { -700.f, 180.f }, { -760.f, -70.f } },
    };

    const Vec2 kChooseOffset[3] =
    {
        { -110.f, 45.f },
        { -55.f, 45.f },
        {  5.f, 45.f },
    };


    inline RECT LockSrc()
    {
        return MakeAtlasRect(768, 128, kLockW, kLockH);
    }

    const wchar_t* GetPlayerTypeNameW(uint8 playerType)
    {
        switch (static_cast<PlayerType>(playerType))
        {
        case PlayerType::Rudwig:  return L"RUDWIG";
        case PlayerType::Ibanix:  return L"IBANIX";
        case PlayerType::Fanthor: return L"FANTHOR";
        default: return L"UNKNOWN";
        }
    }

    const wchar_t* GetRoomErrorNameW(uint8 code)
    {
        switch (static_cast<RoomErrorCode>(code))
        {
        case RoomErrorCode::NotHost:          return L"HOST ONLY";
        case RoomErrorCode::NotAllReady:      return L"WAITING FOR READY";
        case RoomErrorCode::NotEnoughPlayers: return L"NOT ENOUGH PLAYERS";
        case RoomErrorCode::InvalidRoom:      return L"INVALID ROOM";
        case RoomErrorCode::RoomFull:         return L"ROOM FULL";
        case RoomErrorCode::AlreadyInRoom:    return L"ALREADY IN ROOM";
        case RoomErrorCode::RoomInGame:       return L"ROOM IN GAME";
        default:                              return L"UNKNOWN ERROR";
        }
    }

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

    mHeaderText = CreateText({ 40.f, 120.f }, { 900.f, 90.f }, Anchor::TopLeft, L"ROOM --", mColWhite);
    if (auto* tr = mWorld->GetComponent<UITransformComponent>(mHeaderText))
    {
        tr->mPivot = Vec2(0.f, 0.5f);
        tr->mScale = Vec2(1.35f, 1.35f);
    }

    mStatusText = CreateText({ 40.f, 205.f }, { 900.f, 60.f }, Anchor::TopLeft, L"", mColDim);
    if (auto* tr = mWorld->GetComponent<UITransformComponent>(mStatusText))
        tr->mPivot = Vec2(0.f, 0.5f);
    SetVisible(mStatusText, false);


    for (const CharacterUiLayout& layout : kCharacterUiLayouts)
    {
        CharacterOverlayUI& overlay = mCharacterOverlays[layout.playerType];
        overlay.info = CreateAtlasSprite(layout.infoPos, { 320.f, 256.f }, Anchor::Center, layout.infoSrc, 5);
        overlay.ready = CreateAtlasSprite(layout.readyPos, { 360.f, 144.f }, Anchor::Center, layout.readySrc, 8);

        for (uint8 playerIndex = 0; playerIndex < kLobbyPlayerCount; ++playerIndex)
        {
            const Vec2 choosePos =
            {
                layout.infoPos.x + kChooseOffset[playerIndex].x,
                layout.infoPos.y + kChooseOffset[playerIndex].y
            };
            overlay.choose[playerIndex] = CreateAtlasSprite(choosePos, { 64.f, 64.f }, Anchor::Center, layout.chooseSrc, 4);
            SetVisible(overlay.choose[playerIndex], false);


            overlay.lock[playerIndex] = CreateAtlasSprite(choosePos, { 64.f, 64.f }, Anchor::Center, LockSrc(), 10);
            SetVisible(overlay.lock[playerIndex], false);
        }

        SetVisible(overlay.ready, false);
    }

    const Vec2 btnSize = { 384.f, 128.f };

    mReadyButton = CreateUIButton(mWorld, {
        .anchor       = Anchor::BottomRight,
        .position     = Vec2(-230.f, -360.f),
        .size         = btnSize,
        .visual       = UIButtonVisual::Texture,
        .resKey       = L"UI_Robby_Sheet",
		.sourceRect   = MakeAtlasRect(1152, 384, 384, 128),
        .label        = L"READY",
        .normalColor  = mBtnNormal,
        .hoveredColor = mBtnHovered,
        .pressedColor = mBtnPressed,
        .onClick      = [this]()
        {
            LobbyRoomStateComponent* st = GetState();
            bool cur = false;
            if (st)
            {
                const uint32 me = Network::GetInstance().mClientId;
                for (const auto& s : st->mSlots)
                {
                    if (s.sessionId != 0 && s.sessionId == me)
                    {
                        cur = s.ready;
                        break;
                    }
                }
            }
            if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomReadyChanged{ !cur });
        },
    });

    mStartButton = CreateUIButton(mWorld, {
        .anchor       = Anchor::BottomRight,
        .position     = Vec2(-230.f, -240.f),
        .size         = btnSize,
        .visual       = UIButtonVisual::Texture,
        .resKey       = L"UI_Robby_Sheet",
        .sourceRect   = MakeAtlasRect(1152, 384, 384, 128),
        .label        = L"START",
        .normalColor  = mBtnNormal,
        .hoveredColor = mBtnHovered,
        .pressedColor = mBtnPressed,
        .onClick      = [this]()
        {
            LobbyRoomStateComponent* st = GetState();
            if (!st || !st->mHasSnapshot)
                return;

            const uint32 me = Network::GetInstance().mClientId;
            const uint8 cnt = (st->mPlayerCount < ROOM_MAX_PLAYERS) ? st->mPlayerCount : ROOM_MAX_PLAYERS;
            bool host = false;
            bool all = (cnt > 0);
            for (uint8 i = 0; i < cnt; ++i)
            {
                const auto& s = st->mSlots[i];
                if (s.sessionId == me && s.isHost)
                    host = true;
                if (!s.ready)
                    all = false;
            }
            if (host && all)
            {
                if (auto em = mWorld->GetEventManager())
                    em->Enqueue(EvNetSceneChange{ SceneId::Plaza });	// 게임 시작 = 광장으로 출발
                mStarting = true;
            }
        },
    });

    mExitButton = CreateUIButton(mWorld, {
        .anchor       = Anchor::BottomRight,
        .position     = Vec2(-230.f, -120.f),
        .size         = btnSize,
        .visual       = UIButtonVisual::Texture,
        .resKey       = L"UI_Robby_Sheet",
        .sourceRect   = MakeAtlasRect(1152, 384, 384, 128),
        .label        = L"EXIT",
        .normalColor  = mBtnNormal,
        .hoveredColor = mBtnHovered,
        .pressedColor = mBtnPressed,
        .onClick      = [this]()
        {
            if (mLeaving || mStarting)
                return;

            LobbyRoomListComponent* listState = GetListState();
            if (!listState || listState->mCurrentRoomId == 0)
                return;

            // 서버에 방 나가기
            if (auto em = mWorld->GetEventManager())
                em->Enqueue(EvRoomLeave{ listState->mCurrentRoomId });

            listState->mCurrentRoomId = 0;
            if (auto* st = GetState())
                st->mHasSnapshot = false;

            mLeaving = true;
        },
        .clickSfxKey  = "ui/back",
    });

    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mReadyButton))
        sp->mColorTint = mBtnNormal;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mStartButton))
        sp->mColorTint = mBtnNormal;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mExitButton))
        sp->mColorTint = mBtnNormal;

    mStartReasonText = CreateText({ 0.f, -60.f }, { 900.f, 60.f }, Anchor::BottomCenter, L"", mColNotReady);
    mErrorToastText = CreateText({ 0.f, 350.f }, { 1200.f, 80.f }, Anchor::Center, L"", mColNotReady);
    SetVisible(mStartReasonText, false);
    SetVisible(mErrorToastText, false);

    mUiBuilt = true;
}

void LobbyRoomSystem::Update(float dt)
{
    
    if (mLeaving)
    {
        mLeaving = false;
        mWasInRoom = false;
        gEngine->GetSceneManager().RequestScene(SceneId::MainMenu);
        // 방 나가기 송신한 뒤 메인메뉴로 복귀
        return;
    }

    // ImGui용 방탈출
    if (!mStarting)
    {
        LobbyRoomListComponent* listState = GetListState();
        const bool inRoom = (listState != nullptr) && (listState->mCurrentRoomId != 0);
        if (mWasInRoom && !inRoom)
            mLeaving = true;
        mWasInRoom = inRoom;
    }

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
        if (mErrorRemain <= 0.f)
            mLastErrorCode = 0;
    }

    if (!mUiBuilt)
        return;

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
    const bool showToast = (mLastErrorCode != 0) && (mErrorRemain > 0.f);
    SetVisible(mErrorToastText, showToast);

    if (showToast)
        SetText(mErrorToastText, GetRoomErrorNameW(mLastErrorCode));

    LobbyRoomListComponent* listState = GetListState();
    const bool inRoom = (listState != nullptr) && (listState->mCurrentRoomId != 0);
    SetWaitingRoomVisible(inRoom);
    if (!inRoom)
        return;

    const uint32 myClientId = Network::GetInstance().mClientId;
    const bool hasSnap = (state != nullptr) && state->mHasSnapshot;
    const uint8 count = state ? ((state->mPlayerCount < kLobbyUiSlotCount) ? state->mPlayerCount : kLobbyUiSlotCount) : 0;

    bool isMeHost = false;
    bool myReady = false;
    bool allReady = hasSnap && (count > 0);

    for (uint8 i = 0; i < kLobbyUiSlotCount; ++i)
    {
        if (i >= count)
            continue;

        const bool occupied = hasSnap && (i < count) && (state->mSlots[i].sessionId != 0);
        if (!occupied)
        {
            allReady = false;
            continue;
        }

        const auto& slot = state->mSlots[i];
        const bool isMe = (slot.sessionId == myClientId);
        if (isMe)
        {
            myReady = slot.ready;
            if (slot.isHost)
                isMeHost = true;
        }
        if (!slot.ready)
            allReady = false;
    }

    {
        const unsigned maxDisplay =
            (state && state->mMaxPlayers >= 1 && state->mMaxPlayers <= ROOM_MAX_PLAYERS)
            ? state->mMaxPlayers : ROOM_MAX_PLAYERS;

        wchar_t buf[96];
        swprintf_s(buf, L"ROOM%u (%u / %u)",
            state ? state->mRoomId : 0u,
            static_cast<unsigned>(state ? state->mPlayerCount : 0),
            maxDisplay);
        SetText(mHeaderText, buf);
    }

    RefreshCharacterOverlay(state, hasSnap, count, myClientId);

    SetText(mReadyButton, myReady ? L"CANCEL READY" : L"READY");
    SetTextColor(mReadyButton, mColWhite);

    if (auto* btn = mWorld->GetComponent<UIButtonComponent>(mReadyButton))
    {
        btn->mNormalColor = mBtnNormal;
        btn->mHoveredColor = mBtnHovered;
        btn->mPressedColor = mBtnPressed;
        if (!btn->mHovered && !btn->mPressed)
        {
            if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mReadyButton))
                sp->mColorTint = mBtnNormal;
        }
    }

    const bool canStart = isMeHost && allReady && hasSnap;
    if (auto* btn = mWorld->GetComponent<UIButtonComponent>(mStartButton))
        btn->mEnabled = canStart;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(mStartButton))
        sp->mColorTint = canStart ? mBtnNormal : mBtnDisabled;
    SetTextColor(mStartButton, canStart ? mColWhite : mColDim);

    const wchar_t* reason = nullptr;
    if (!hasSnap)
        reason = L"NEED ROOM STATE";
    else if (!isMeHost)
        reason = L"HOST ONLY";
    else if (!allReady)
        reason = L"WAITING FOR READY";

    SetVisible(mStartReasonText, reason != nullptr);
    if (reason)
        SetText(mStartReasonText, reason);
}

void LobbyRoomSystem::RefreshCharacterOverlay(LobbyRoomStateComponent* state, bool hasSnap, uint8 count, uint32 myClientId)
{
    // 기본 정보 
    for (CharacterOverlayUI& overlay : mCharacterOverlays)
    {
        SetVisible(overlay.info, true);
        SetVisible(overlay.ready, false);
        for (Entity choose : overlay.choose)
            SetVisible(choose, false);
        for (Entity lock : overlay.lock)
            SetVisible(lock, false);
    }

    // 스냅샷과 무관하게 ready/choose/lock 을 강제로 보이게 함 (디버그용)
    if (mDebugForceShowOverlays)
    {
        for (CharacterOverlayUI& overlay : mCharacterOverlays)
        {
            SetVisible(overlay.ready, true);
            for (Entity choose : overlay.choose)
                SetVisible(choose, true);
            for (Entity lock : overlay.lock)
                SetVisible(lock, true);
        }
        return;
    }

    if (!hasSnap || state == nullptr)
        return;

    // 현재 선택(호버)한 캐릭터.
    uint8 myLiveType = 0xFF;
    if (mWorld->HasComponentPool<ChoicePlayerComponent>())
    {
        auto choiceEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
        if (!choiceEntities.empty())
            if (auto* choice = mWorld->GetComponent<ChoicePlayerComponent>(choiceEntities[0]))
                myLiveType = choice->mPlayerType;
    }

    // 캐릭터별로 누가 선택했는지/누가 레디 확정했는지 집계
    bool selected[kLobbyCharacterCount][kLobbyPlayerCount] = {};
    int  readyByIndex[kLobbyCharacterCount];
    for (int& r : readyByIndex)
        r = -1;

    for (uint8 slotIndex = 0; slotIndex < count && slotIndex < kLobbyPlayerCount; ++slotIndex)
    {
        const auto& slot = state->mSlots[slotIndex];
        if (slot.sessionId == 0)
            continue;

        const bool isMe = (slot.sessionId == myClientId);
        // 선택 캐릭터: 본인은 실시간 선택값을, 원격은 스냅샷 값을 사용
        const uint8 type = (isMe && myLiveType < kLobbyCharacterCount) ? myLiveType : slot.playerType;
        if (type >= kLobbyCharacterCount)
            continue;

        selected[type][slotIndex] = true;
        if (slot.ready) // 한 캐릭터 레디
            readyByIndex[type] = slotIndex;   
    }

    // 잠금 표시
    for (uint8 c = 0; c < kLobbyCharacterCount; ++c)
    {
        CharacterOverlayUI& overlay = mCharacterOverlays[c];
        const int readyIdx = readyByIndex[c];

        // Ready : 누군가 이 캐릭터를 레디 확정하면 표시
        if (readyIdx >= 0)
            SetVisible(overlay.ready, true);

        for (uint8 i = 0; i < kLobbyPlayerCount; ++i)
        {
            // 레디 확정된 캐릭터의 다른 p위치 잠금(X)
            const bool locked = (readyIdx >= 0) && (i != readyIdx);
            if (locked)
                SetVisible(overlay.lock[i], true);
            else if (selected[c][i])
                SetVisible(overlay.choose[i], true);   // 선택(호버) 토큰
        }
    }
}



void LobbyRoomSystem::SetWaitingRoomVisible(bool visible)
{
    SetVisible(mHeaderText, visible);
    SetVisible(mStatusText, false);

    for (SlotCardUI& c : mSlotCards)
    {
        SetVisible(c.bg, false);
        SetVisible(c.nameText, false);
        SetVisible(c.charText, false);
        SetVisible(c.readyText, false);
        SetVisible(c.hostText, false);
        SetVisible(c.youText, false);
    }

    for (CharacterOverlayUI& overlay : mCharacterOverlays)
    {
        SetVisible(overlay.info, visible);
        SetVisible(overlay.ready, false);
        for (Entity choose : overlay.choose)
            SetVisible(choose, false);
        for (Entity lock : overlay.lock)
            SetVisible(lock, false);
    }

    SetVisible(mReadyButton, visible);
    SetVisible(mStartButton, visible);
    SetVisible(mExitButton, visible);
    SetVisible(mStartReasonText, visible);

    if (auto* b = mWorld->GetComponent<UIButtonComponent>(mReadyButton))
        b->mEnabled = visible;
    if (auto* b = mWorld->GetComponent<UIButtonComponent>(mStartButton))
        b->mEnabled = visible;
    if (auto* b = mWorld->GetComponent<UIButtonComponent>(mExitButton))
        b->mEnabled = visible;
}




Entity LobbyRoomSystem::CreateText(const Vec2& pos, const Vec2& size, Anchor anchor,
                                   const std::wstring& text, const Vec4& color)
{
    Entity e = mWorld->CreateEntity();
    auto& tr = mWorld->AddComponent<UITransformComponent>(e);
    tr.mAnchor = anchor;
    tr.mPosition = pos;
    tr.mSize = size;
    tr.mPivot = Vec2(0.5f, 0.5f);
    tr.mUILayerIndex = 6;

    auto& txt = mWorld->AddComponent<UITextComponent>(e);
    txt.mText = text;
    txt.mColor.f[0] = color.x;
    txt.mColor.f[1] = color.y;
    txt.mColor.f[2] = color.z;
    txt.mColor.f[3] = color.w;
    return e;
}

Entity LobbyRoomSystem::CreateAtlasSprite(const Vec2& pos, const Vec2& size, Anchor anchor,
                                          const RECT& srcRect, uint8 layer, Vec4 tint)
{
    Entity e = mWorld->CreateEntity();
    auto& tr = mWorld->AddComponent<UITransformComponent>(e);
    tr.mAnchor = anchor;
    tr.mPosition = pos;
    tr.mSize = size;
    tr.mPivot = Vec2(0.5f, 0.5f);
    tr.mUILayerIndex = layer;

    auto& sp = mWorld->AddComponent<UISpriteComponent>(e, RESOURCEMANAGER.Get<Texture>(mSheetName));
    sp.SetSourceRect(static_cast<float>(srcRect.left),
                     static_cast<float>(srcRect.top),
                     static_cast<float>(srcRect.right - srcRect.left),
                     static_cast<float>(srcRect.bottom - srcRect.top));
    sp.mColorTint = tint;
    return e;
}


void LobbyRoomSystem::GetOverlayEntities(uint8 idx, Entity& info, Entity& ready,
    std::array<Entity, 3>& choose,
    std::array<Entity, 3>& lock) const
{
    info = Entity{};
    ready = Entity{};
    choose = {};
    lock = {};
    if (idx >= kLobbyCharacterCount)
        return;

    const CharacterOverlayUI& overlay = mCharacterOverlays[idx];
    info = overlay.info;
    ready = overlay.ready;
    for (uint8 i = 0; i < kLobbyPlayerCount && i < choose.size(); ++i)
    {
        choose[i] = overlay.choose[i];
        lock[i] = overlay.lock[i];
    }
}

LobbyRoomStateComponent* LobbyRoomSystem::GetState()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomStateComponent>())
        return nullptr;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
    if (entities.empty())
        return nullptr;

    return mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]);
}

LobbyRoomListComponent* LobbyRoomSystem::GetListState()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomListComponent>())
        return nullptr;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty())
        return nullptr;

    return mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
}


void LobbyRoomSystem::SetText(Entity e, const std::wstring& text)
{
    if (!e.IsValid())
        return;
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
        tx->mText = text;
}

void LobbyRoomSystem::SetTextColor(Entity e, const Vec4& color)
{
    if (!e.IsValid())
        return;
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
    {
        tx->mColor.f[0] = color.x;
        tx->mColor.f[1] = color.y;
        tx->mColor.f[2] = color.z;
        tx->mColor.f[3] = color.w;
    }
}

void LobbyRoomSystem::SetVisible(Entity e, bool visible)
{
    if (!e.IsValid())
        return;
    if (auto* tx = mWorld->GetComponent<UITextComponent>(e))
        tx->mVisible = visible;
    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e))
        sp->mVisible = visible;
}

