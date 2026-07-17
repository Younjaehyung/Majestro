#include "pch.h"
#include "RoomPacketHandler.h"
#include "RoomManager.h"
#include "RoomNotifier.h"
#include "ServerCore.h"

bool RoomPacketHandler::Handle(const InputCommand& command)
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
        return HandleReady(command);
    case PKT_Type::C2S_ROOM_CHARACTER_SELECT:
        return HandleCharacterSelect(command);
    default:
        return false;
    }
}

void RoomPacketHandler::HandleCreate(uint64 sessionId)
{
    uint32 roomId = 0;
    const RoomErrorCode err = mRoomManager->CreateRoom(sessionId, roomId);
    if (err != RoomErrorCode::None)
    {
        mNotifier->SendJoinResult(sessionId, 0, false, err);
        return;
    }

    mNotifier->SendJoinResult(sessionId, roomId, true, RoomErrorCode::None);
    mNotifier->BroadcastRoomState(roomId);        // 본인에게 방 상태
    mNotifier->BroadcastRoomListToHall();         // 홀 사람들 목록 갱신
}

void RoomPacketHandler::HandleJoin(uint64 sessionId, uint32 roomId)
{
    const RoomErrorCode err = mRoomManager->JoinRoom(sessionId, roomId);
    if (err != RoomErrorCode::None)
    {
        mNotifier->SendJoinResult(sessionId, 0, false, err);
        return;
    }

    mNotifier->SendJoinResult(sessionId, roomId, true, RoomErrorCode::None);
    mNotifier->BroadcastRoomState(roomId);        // 방원 전원 상태 갱신
    mNotifier->BroadcastRoomListToHall();
}

void RoomPacketHandler::HandleLeave(uint64 sessionId)
{
    uint32 roomId = 0;
    if (!mRoomManager->LeaveRoom(sessionId, roomId)) return;

    if (mRoomManager->GetRoom(roomId) != nullptr)  // 방이 남아 있으면 잔류 인원에게 상태 갱신
        mNotifier->BroadcastRoomState(roomId);

    mNotifier->BroadcastRoomListToHall();
    mNotifier->SendRoomList(sessionId);            // 홀로 돌아온 본인에게 목록
}

void RoomPacketHandler::HandleList(uint64 sessionId)
{
    mNotifier->SendRoomList(sessionId);

    // 요청자가 아직 게임에서 나가지 않고 방에 들어가 있는 상태였다면 복구시킴
    const uint32 roomId = mRoomManager->GetRoomIdByPlayer(sessionId);
    if (roomId != 0)
        mNotifier->BroadcastRoomState(roomId);
}

bool RoomPacketHandler::HandleReady(const InputCommand& command)
{
    const C2S_RoomReadyPacket* pkt = command.ViewAs<C2S_RoomReadyPacket>();
    if (pkt == nullptr) return false;

    const uint64 sessionId = command.SessionId;
    RoomState* room = mRoomManager->GetRoomByPlayer(sessionId);
    if (room == nullptr)
    {
        mNotifier->SendError(sessionId, 0, RoomErrorCode::InvalidRoom);
        return true;
    }
    if (pkt->roomId != room->mRoomId)
    {
        mNotifier->SendError(sessionId, pkt->roomId, RoomErrorCode::InvalidRoom);
        return true;
    }

 
    const RoomErrorCode selectionError = mRoomManager->SelectRhythmMusic(
        sessionId,
        SanitizeRhythmMusicVariant(pkt->rhythmMusicSubVariant));
    if (selectionError != RoomErrorCode::None)
    {
        mNotifier->SendError(sessionId, room->mRoomId, selectionError);
        return true;
    }

    const RoomErrorCode err = mRoomManager->SetPlayerReady(sessionId, pkt->ready != 0);
    if (err != RoomErrorCode::None)
    {
        // 이미 확정된 캐릭터로 Ready 시도시 거부 통지 + 상태 재동기화
        mNotifier->SendError(sessionId, room->mRoomId, err);
    }
    mNotifier->BroadcastRoomState(room->mRoomId);
    return true;
}

bool RoomPacketHandler::HandleCharacterSelect(const InputCommand& command)
{
    const C2S_RoomCharacterSelectPacket* pkt = command.ViewAs<C2S_RoomCharacterSelectPacket>();
    if (pkt == nullptr) return false;

    const uint64 sessionId = command.SessionId;
    RoomState* room = mRoomManager->GetRoomByPlayer(sessionId);
    if (room == nullptr)
    {
        mNotifier->SendError(sessionId, 0, RoomErrorCode::InvalidRoom);
        return true;
    }
    if (pkt->roomId != room->mRoomId)
    {
        mNotifier->SendError(sessionId, pkt->roomId, RoomErrorCode::InvalidRoom);
        return true;
    }

    // 실패(다른 ready 플레이어가 확정한 캐릭터)여도 오류 통지 없이 상태 재동기화만 수행
    mRoomManager->SelectCharacter(sessionId, pkt->playerType);
    mNotifier->BroadcastRoomState(room->mRoomId);
    return true;
}
