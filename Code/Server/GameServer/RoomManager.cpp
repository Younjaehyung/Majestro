#include "pch.h"
#include "RoomManager.h"
#include "RoomNotifier.h"
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
        if (mNotifier) mNotifier->SendRoomList(sessionId);
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
        if (mNotifier) mNotifier->BroadcastRoomState(roomId);

    if (mNotifier) mNotifier->BroadcastRoomListToHall();
}

void RoomManager::OnGameStarted(uint64 sessionId)
{
    RoomState* room = GetRoomByPlayer(sessionId);
    if (room == nullptr) return;

    // 게임 중 상태로 전환
    room->mPhase = RoomPhase::InGame;
    if (mNotifier)
    {
        mNotifier->BroadcastRoomState(room->mRoomId);
        mNotifier->BroadcastRoomListToHall();   // 목록의 phase 갱신
    }
}

RoomErrorCode RoomManager::CreateRoom(uint64 sessionId, uint32& outRoomId)
{
    outRoomId = 0;
    if (mSessionToRoom.find(sessionId) != mSessionToRoom.end())
        return RoomErrorCode::AlreadyInRoom;

    const uint32 roomId = mNextRoomId++;
    RoomState& room = mRooms[roomId];
    room.mRoomId = roomId;
    room.mPhase = RoomPhase::Waiting;
    room.AddPlayer(sessionId);   // 첫 입장자 = Host

    mSessionToRoom[sessionId] = roomId;
    mHallSessions.erase(sessionId);

    outRoomId = roomId;
    return RoomErrorCode::None;
}

RoomErrorCode RoomManager::JoinRoom(uint64 sessionId, uint32 roomId)
{
    if (mSessionToRoom.find(sessionId) != mSessionToRoom.end())
        return RoomErrorCode::AlreadyInRoom;

    RoomState* room = GetRoom(roomId);
    if (room == nullptr)
        return RoomErrorCode::InvalidRoom;
    if (room->mPhase == RoomPhase::InGame)
        return RoomErrorCode::RoomInGame;
    if (room->mPlayers.size() >= ROOM_MAX_PLAYERS)
        return RoomErrorCode::RoomFull;

    room->AddPlayer(sessionId);
    mSessionToRoom[sessionId] = roomId;
    mHallSessions.erase(sessionId);
    return RoomErrorCode::None;
}

bool RoomManager::LeaveRoom(uint64 sessionId, uint32& outRoomId)
{
    outRoomId = 0;
    auto findIt = mSessionToRoom.find(sessionId);
    if (findIt == mSessionToRoom.end()) return false;

    outRoomId = findIt->second;
    mSessionToRoom.erase(findIt);
    mHallSessions.insert(sessionId);

    RoomState* room = GetRoom(outRoomId);
    if (room != nullptr)
    {
        room->RemovePlayer(sessionId);   // Host 빠지면 승계
        if (room->mPlayers.empty())
            mRooms.erase(outRoomId);     // 빈 방 소멸
    }
    return true;
}

RoomErrorCode RoomManager::SetPlayerReady(uint64 sessionId, bool ready)
{
    RoomState* room = GetRoomByPlayer(sessionId);
    if (room == nullptr)
        return RoomErrorCode::InvalidRoom;

    if (!room->SetReady(sessionId, ready))
        return RoomErrorCode::CharacterTaken;

    // 확정 성공 시, 같은 캐릭터를 고르던 미확정 플레이어들을 빈 캐릭터로 밀어냄
    if (ready)
    {
        uint8 lockedType = 0;
        if (room->GetPlayerType(sessionId, lockedType))
            room->EvictConflictingSelections(sessionId, lockedType);
    }
    return RoomErrorCode::None;
}

RoomErrorCode RoomManager::SelectCharacter(uint64 sessionId, uint8 playerType)
{
    RoomState* room = GetRoomByPlayer(sessionId);
    if (room == nullptr)
        return RoomErrorCode::InvalidRoom;

    if (!room->SetPlayerCharacter(sessionId, playerType))
        return RoomErrorCode::CharacterTaken;
    return RoomErrorCode::None;
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
