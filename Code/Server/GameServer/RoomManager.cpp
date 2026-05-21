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
    entry.playerType = 0;
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
            player.ready = ready;
            return true;
        }
    }
    return false;
}

bool RoomState::SetCharacter(uint64 sessionId, uint8 playerType)
{
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

void RoomState::ResetAllReady()
{
    for (auto& player : mPlayers)
        player.ready = false;
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
}

void RoomManager::OnSessionEnterLobby(uint64 sessionId)
{
    // 이미 매핑돼 있으면 중복 입장 방지 (예: 게임 종료 후 다시 InitializeSession 이 와도 안전)
    if (mSessionToRoom.find(sessionId) != mSessionToRoom.end())
        return;

    RoomState& room = mRooms[mDefaultRoomId];
    room.mRoomId = mDefaultRoomId;
    if (!room.AddPlayer(sessionId))
        return;  // 방이 꽉 찼거나 중복

    mSessionToRoom[sessionId] = mDefaultRoomId;
    BroadcastRoomState(mDefaultRoomId);
}

void RoomManager::OnSessionLeave(uint64 sessionId)
{
    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end()) return;

    const uint32 roomId = findIt->second;
    mSessionToRoom.erase(findIt);

    RoomState* room = GetRoom(roomId);
    if (room == nullptr) return;

    room->RemovePlayer(sessionId);
    BroadcastRoomState(roomId);
}

bool RoomManager::HandleRoomPacket(const InputCommand& command)
{
    const uint64 sessionId = command.SessionId;
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
        if (!room->SetReady(sessionId, pkt->ready != 0)) return false;
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
        if (!room->SetCharacter(sessionId, pkt->playerType)) return false;
        BroadcastRoomState(room->mRoomId);
        return true;
    }

    return false;
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

    // 게임 종료 후 로비 복귀 ready 해제
    room->ResetAllReady();
    BroadcastRoomState(room->mRoomId);
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

void RoomManager::SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code)
{
    S2C_RoomErrorPacket errPkt;
    errPkt.roomId = roomId;
    errPkt.errorCode = static_cast<uint8>(code);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_ERROR, sizeof(S2C_RoomErrorPacket) };
    req.StoreAs<S2C_RoomErrorPacket>(errPkt);
    gSendQueue.Push(req);
}
