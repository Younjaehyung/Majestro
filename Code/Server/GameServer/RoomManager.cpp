#include "pch.h"
#include "RoomManager.h"
#include "ServerCore.h"

// RoomState
bool RoomState::AddPlayer(uint64 sessionId)
{
    // 중복 입장 방지: 같은 sessionId 가 있으면 false
    for (const auto& player : mPlayers)
        if (player.sessionId == sessionId) return false;

    if (mPlayers.size() >= ROOM_MAX_PLAYERS) return false;

    RoomPlayerEntry entry{};
    entry.sessionId = sessionId;
    // 빈 캐릭터를 기본 배정
    entry.playerType = PickFreeCharacter(sessionId);
    entry.ready = false;
    entry.isHost = (mHostSessionId == 0);  // 빈 방이면 첫 입장자가 Host

    if (entry.isHost)
        mHostSessionId = sessionId;

    mPlayers.push_back(entry);
    return true;
}

bool RoomState::RemovePlayer(uint64 sessionId)
{
    auto findIt = std::find_if(mPlayers.begin(), mPlayers.end(),
        [sessionId](const RoomPlayerEntry& p) { return p.sessionId == sessionId; });

    if (findIt == mPlayers.end()) return false;

    const bool wasHost = findIt->isHost;
    mPlayers.erase(findIt);

    if (wasHost)
    {
        // Host 승계: 남은 첫 슬롯이 Host. 비어 있으면 Host 없음.
        mHostSessionId = mPlayers.empty() ? 0 : mPlayers.front().sessionId;
        if (!mPlayers.empty())
            mPlayers.front().isHost = true;
    }
    return true;
}

bool RoomState::SetReady(uint64 sessionId, bool ready)
{
    for (auto& player : mPlayers)
    {
        if (player.sessionId == sessionId)
        {
            // Ready 확정 시, 내 캐릭터가 이미 다른 ready 플레이어와 겹치면 거부.
            // (해제(ready=false)는 항상 허용)
            if (ready && IsCharacterLockedByOtherReadyPlayer(sessionId, player.playerType))
                return false;
            player.ready = ready;
            return true;
        }
    }
    return false;
}

bool RoomState::SetPlayerCharacter(uint64 sessionId, uint8 playerType)
{
    if (IsCharacterLockedByOtherReadyPlayer(sessionId, playerType))
        return false;

    for (auto& player : mPlayers)
    {
        if (player.sessionId == sessionId)
        {
            player.playerType = playerType;
            // 캐릭터 바꾸면 본인의 ready 는 자동 해제
            player.ready = false;
            return true;
        }
    }
    return false;
}

uint8 RoomState::PickFreeCharacter(uint64 sessionId) const
{
    // 자신을 제외한 아무도 고르지 않은 캐릭터로 부여
    for (uint8 t = 0; t < ROOM_CHARACTER_COUNT; ++t)
    {
        bool taken = false;
        for (const auto& p : mPlayers)
            if (p.sessionId != 0 && p.sessionId != sessionId && p.playerType == t) { taken = true; break; }
        if (!taken) return t;
    }
    // ready 로 잠기지 않은 캐릭터(미확정자끼리는 겹쳐도 허용)
    for (uint8 t = 0; t < ROOM_CHARACTER_COUNT; ++t)
        if (!IsCharacterLockedByOtherReadyPlayer(sessionId, t))
            return t;
    return 0;  // 도달 불가
}

void RoomState::EvictConflictingSelections(uint64 lockerSessionId, uint8 lockedType)
{
    for (auto& player : mPlayers)
    {
        if (player.sessionId == 0) continue;
        if (player.sessionId == lockerSessionId) continue;
        if (player.ready) continue;                     // 이미 확정한 사람은 건드리지 않음
        if (player.playerType != lockedType) continue;  // 겹치는 미확정자만 이동

        player.playerType = PickFreeCharacter(player.sessionId);
    }
}

bool RoomState::GetPlayerType(uint64 sessionId, uint8& outType) const
{
    for (const auto& player : mPlayers)
    {
        if (player.sessionId == sessionId)
        {
            outType = player.playerType;
            return true;
        }
    }
    return false;
}

bool RoomState::IsCharacterLockedByOtherReadyPlayer(uint64 sessionId, uint8 playerType) const
{
    for (const auto& player : mPlayers)
    {
        if (player.sessionId != 0 &&player.sessionId != sessionId &&
            player.ready && player.playerType == playerType)
        {
            return true;
        }
    }

    return false;
}

bool RoomState::IsAllReady() const
{
    if (mPlayers.empty()) return false;
    for (const auto& player : mPlayers)
        if (!player.ready) return false;
    return true;
}

bool RoomState::IsHost(uint64 sessionId) const
{
    return mHostSessionId == sessionId;
}

void RoomState::FillStatePacket(S2C_RoomStatePacket& outPacket) const
{
    outPacket.roomId = mRoomId;
    outPacket.playerCount = static_cast<uint8>(mPlayers.size());
    outPacket.maxPlayers = ROOM_MAX_PLAYERS;
    outPacket.hostSlotIndex = 0xFF;
    outPacket.reserved = 0;

    for (uint8 slotIdx = 0; slotIdx < ROOM_MAX_PLAYERS; ++slotIdx)
        outPacket.slots[slotIdx] = RoomPlayerSlot{};

    for (size_t i = 0; i < mPlayers.size() && i < ROOM_MAX_PLAYERS; ++i)
    {
        const auto& src = mPlayers[i];
        RoomPlayerSlot& dst = outPacket.slots[i];
        dst.sessionId = static_cast<uint32>(src.sessionId);
        dst.playerType = src.playerType;
        dst.ready = src.ready ? 1 : 0;
        dst.isHost = src.isHost ? 1 : 0;
        if (src.isHost)
            outPacket.hostSlotIndex = static_cast<uint8>(i);
    }
}

void RoomState::FillListEntry(RoomListEntry& out) const
{
    out.roomId = mRoomId;
    out.playerCount = static_cast<uint8>(mPlayers.size());
    out.maxPlayers = ROOM_MAX_PLAYERS;
    out.phase = static_cast<uint8>(mPhase);
    out.reserved = 0;
}

std::vector<uint64> RoomState::GetSessionIds() const
{
    std::vector<uint64> result;
    result.reserve(mPlayers.size());
    for (const auto& player : mPlayers)
        result.push_back(player.sessionId);
    return result;
}

//  RoomManager

void RoomManager::Initialize()
{
    mRooms.clear();
    mSessionToRoom.clear();
    mHallSessions.clear();
    mNextRoomId = 1;
}

void RoomManager::OnSessionEnterLobby(uint64 sessionId)
{
    // 방 목록창 입장
    mHallSessions.insert(sessionId);

    // 아직 플레이라고 판단되는 플레이어를 제외한 사람들에게 방목록 전송
    if (mSessionToRoom.find(sessionId) == mSessionToRoom.end())
        SendRoomList(sessionId);
}

void RoomManager::OnSessionLeave(uint64 sessionId)
{
    // 접속 종료
    mHallSessions.erase(sessionId);

    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end()) return;

    const uint32 roomId = findIt->second;
    mSessionToRoom.erase(findIt);

    RoomState* room = GetRoom(roomId);
    if (room == nullptr) return;

    room->RemovePlayer(sessionId);
    if (room->mPlayers.empty())
        mRooms.erase(roomId);        // 빈 방 소멸
    else
        BroadcastRoomState(roomId);

    BroadcastRoomListToHall();
}

bool RoomManager::HandleRoomPacket(const InputCommand& command)
{
    const uint64 sessionId = command.SessionId;

    switch (command.Type)
    {
    case PKT_Type::C2S_ROOM_CREATE:
        HandleCreate(sessionId);
        return true;
    case PKT_Type::C2S_ROOM_JOIN:
    {
        const C2S_RoomJoinPacket* pkt = command.ViewAs<C2S_RoomJoinPacket>();
        if (pkt == nullptr) return false;
        HandleJoin(sessionId, pkt->roomId);
        return true;
    }
    case PKT_Type::C2S_ROOM_LIST:
        HandleList(sessionId);
        return true;
    case PKT_Type::C2S_ROOM_LEAVE:
        HandleLeave(sessionId);
        return true;
    case PKT_Type::C2S_ROOM_READY:
    case PKT_Type::C2S_ROOM_CHARACTER_SELECT:
        break;  // 아래에서 방 소속 검사 후 처리
    default:
        return false;
    }

    RoomState* room = GetRoomByPlayer(sessionId);
    if (room == nullptr)
    {
        SendError(sessionId, 0, RoomErrorCode::InvalidRoom);
        return true;
    }

    if (command.Type == PKT_Type::C2S_ROOM_READY)
    {
        const C2S_RoomReadyPacket* pkt = command.ViewAs<C2S_RoomReadyPacket>();
        if (pkt == nullptr) return false;
        if (pkt->roomId != room->mRoomId)
        {
            SendError(sessionId, pkt->roomId, RoomErrorCode::InvalidRoom);
            return true;
        }
        if (!room->SetReady(sessionId, pkt->ready != 0))
        {
            // 이미 확정된 캐릭터로 Ready 시도시 거부 통지 + 상태 재동기화
            SendError(sessionId, room->mRoomId, RoomErrorCode::CharacterTaken);
            BroadcastRoomState(room->mRoomId);
            return true;
        }
        // 확정 성공 시, 같은 캐릭터를 고르던 미확정 플레이어들을 빈 캐릭터로 밀어냄
        if (pkt->ready != 0)
        {
            uint8 lockedType = 0;
            if (room->GetPlayerType(sessionId, lockedType))
                room->EvictConflictingSelections(sessionId, lockedType);
        }
        BroadcastRoomState(room->mRoomId);
        return true;
    }

    if (command.Type == PKT_Type::C2S_ROOM_CHARACTER_SELECT)
    {
        const C2S_RoomCharacterSelectPacket* pkt = command.ViewAs<C2S_RoomCharacterSelectPacket>();
        if (pkt == nullptr) return false;
        if (pkt->roomId != room->mRoomId)
        {
            SendError(sessionId, pkt->roomId, RoomErrorCode::InvalidRoom);
            return true;
        }
        if (!room->SetPlayerCharacter(sessionId, pkt->playerType))
        {
            BroadcastRoomState(room->mRoomId);
            return true;
        }
        BroadcastRoomState(room->mRoomId);
        return true;
    }

    return false;
}

void RoomManager::HandleCreate(uint64 sessionId)
{
    if (mSessionToRoom.find(sessionId) != mSessionToRoom.end())
    {
        SendJoinResult(sessionId, 0, false, RoomErrorCode::AlreadyInRoom);
        return;
    }

    const uint32 roomId = mNextRoomId++;
    RoomState& room = mRooms[roomId];
    room.mRoomId = roomId;
    room.mPhase = RoomPhase::Waiting;
    room.AddPlayer(sessionId);   // 첫 입장자 = Host

    mSessionToRoom[sessionId] = roomId;
    mHallSessions.erase(sessionId);

    SendJoinResult(sessionId, roomId, true, RoomErrorCode::None);
    BroadcastRoomState(roomId);        // 본인에게 방 상태
    BroadcastRoomListToHall();         // 홀 사람들 목록 갱신
}

void RoomManager::HandleJoin(uint64 sessionId, uint32 roomId)
{
    if (mSessionToRoom.find(sessionId) != mSessionToRoom.end())
    {
        SendJoinResult(sessionId, 0, false, RoomErrorCode::AlreadyInRoom);
        return;
    }

    RoomState* room = GetRoom(roomId);
    if (room == nullptr)
    {
        SendJoinResult(sessionId, 0, false, RoomErrorCode::InvalidRoom);
        return;
    }
    if (room->mPhase == RoomPhase::InGame)
    {
        SendJoinResult(sessionId, 0, false, RoomErrorCode::RoomInGame);
        return;
    }
    if (room->mPlayers.size() >= ROOM_MAX_PLAYERS)
    {
        SendJoinResult(sessionId, 0, false, RoomErrorCode::RoomFull);
        return;
    }

    room->AddPlayer(sessionId);
    mSessionToRoom[sessionId] = roomId;
    mHallSessions.erase(sessionId);

    SendJoinResult(sessionId, roomId, true, RoomErrorCode::None);
    BroadcastRoomState(roomId);        // 방원 전원 상태 갱신
    BroadcastRoomListToHall();
}

void RoomManager::HandleLeave(uint64 sessionId)
{
    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end()) return;

    const uint32 roomId = findIt->second;
    mSessionToRoom.erase(findIt);
    mHallSessions.insert(sessionId);

    RoomState* room = GetRoom(roomId);
    if (room != nullptr)
    {
        room->RemovePlayer(sessionId);   // Host 빠지면 승계
        if (room->mPlayers.empty())
            mRooms.erase(roomId);        // 빈 방 소멸
        else
            BroadcastRoomState(roomId);
    }

    BroadcastRoomListToHall();
    SendRoomList(sessionId);             // 홀로 돌아온 본인에게 목록
}

void RoomManager::HandleList(uint64 sessionId)
{
    SendRoomList(sessionId);

    // 요청자가 아직 게임에서 나가지 않고 방에 들어가 있는 상태였다면 복구시킴
    const uint32 roomId = GetRoomIdByPlayer(sessionId);
    if (roomId != 0)
        BroadcastRoomState(roomId);
}

bool RoomManager::CanStartGame(uint64 sessionId, RoomErrorCode& outError) const
{
    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end())
    {
        outError = RoomErrorCode::InvalidRoom;
        return false;
    }

    auto roomIt = mRooms.find(findIt->second);
    if (roomIt == mRooms.end())
    {
        outError = RoomErrorCode::InvalidRoom;
        return false;
    }

    const RoomState& room = roomIt->second;

    if (!room.IsHost(sessionId))
    {
        outError = RoomErrorCode::NotHost;
        return false;
    }

    if (room.mPlayers.size() < mMinPlayersToStart)
    {
        outError = RoomErrorCode::NotEnoughPlayers;
        return false;
    }

    if (!room.IsAllReady())
    {
        outError = RoomErrorCode::NotAllReady;
        return false;
    }

    outError = RoomErrorCode::None;
    return true;
}

void RoomManager::OnGameStarted(uint64 sessionId)
{
    RoomState* room = GetRoomByPlayer(sessionId);
    if (room == nullptr) return;

    // 게임 중 상태로 전환
    room->mPhase = RoomPhase::InGame;
    BroadcastRoomState(room->mRoomId);
    BroadcastRoomListToHall();          // 목록의 phase 갱신
}

RoomState* RoomManager::GetRoomByPlayer(uint64 sessionId)
{
    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end()) return nullptr;
    return GetRoom(findIt->second);
}

RoomState* RoomManager::GetRoom(uint32 roomId)
{
    auto findIt = mRooms.find(roomId);
    if (findIt == mRooms.end()) return nullptr;
    return &findIt->second;
}

uint32 RoomManager::GetRoomIdByPlayer(uint64 sessionId) const
{
    auto findIt = mSessionToRoom.find(sessionId);
    return findIt != mSessionToRoom.end() ? findIt->second : 0;
}

void RoomManager::BroadcastRoomState(uint32 roomId)
{
    RoomState* room = GetRoom(roomId);
    if (room == nullptr) return;

    S2C_RoomStatePacket statePkt;
    room->FillStatePacket(statePkt);

    // 브로드캐스트
    for (uint64 sessionId : room->GetSessionIds())
    {
        SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_STATE, sizeof(S2C_RoomStatePacket) };
        req.StoreAs<S2C_RoomStatePacket>(statePkt);
        gSendQueue.Push(req);
    }
}

void RoomManager::FillRoomListPacket(S2C_RoomListPacket& outPacket) const
{
    uint8 count = 0;
    for (const auto& [roomId, room] : mRooms)
    {
        if (count >= ROOM_LIST_MAX_ENTRIES) break;
        room.FillListEntry(outPacket.entries[count]);
        ++count;
    }
    outPacket.count = count;
}

void RoomManager::BroadcastRoomListToHall()
{
    S2C_RoomListPacket listPkt;
    FillRoomListPacket(listPkt);

    for (uint64 sessionId : mHallSessions)
    {
        SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_LIST, sizeof(S2C_RoomListPacket) };
        req.StoreAs<S2C_RoomListPacket>(listPkt);
        gSendQueue.Push(req);
    }
}

void RoomManager::SendRoomList(uint64 sessionId)
{
    S2C_RoomListPacket listPkt;
    FillRoomListPacket(listPkt);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_LIST, sizeof(S2C_RoomListPacket) };
    req.StoreAs<S2C_RoomListPacket>(listPkt);
    gSendQueue.Push(req);
}

void RoomManager::SendJoinResult(uint64 sessionId, uint32 roomId, bool success, RoomErrorCode code)
{
    S2C_RoomJoinResultPacket resultPkt;
    resultPkt.roomId = roomId;
    resultPkt.success = success ? 1 : 0;
    resultPkt.errorCode = static_cast<uint8>(code);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_JOIN_RESULT, sizeof(S2C_RoomJoinResultPacket) };
    req.StoreAs<S2C_RoomJoinResultPacket>(resultPkt);
    gSendQueue.Push(req);
}

void RoomManager::SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code)
{
    S2C_RoomErrorPacket errPkt;
    errPkt.roomId = roomId;
    errPkt.errorCode = static_cast<uint8>(code);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_ERROR, sizeof(S2C_RoomErrorPacket) };
    req.StoreAs<S2C_RoomErrorPacket>(errPkt);
    gSendQueue.Push(req);
}
