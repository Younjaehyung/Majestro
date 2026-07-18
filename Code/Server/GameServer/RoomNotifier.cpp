#include "pch.h"
#include "RoomNotifier.h"
#include "RoomManager.h"
#include "ServerCore.h"

void RoomNotifier::FillStatePacket(const RoomState& room, S2C_RoomStatePacket& outPacket) const
{
    outPacket.roomId = room.mRoomId;
    outPacket.playerCount = static_cast<uint8>(room.mPlayers.size());
    outPacket.maxPlayers = ROOM_MAX_PLAYERS;
    outPacket.hostSlotIndex = 0xFF;
    outPacket.reserved = 0;

    for (uint8 slotIdx = 0; slotIdx < ROOM_MAX_PLAYERS; ++slotIdx)
        outPacket.slots[slotIdx] = RoomPlayerSlot{};

    for (size_t i = 0; i < room.mPlayers.size() && i < ROOM_MAX_PLAYERS; ++i)
    {
        const auto& src = room.mPlayers[i];
        RoomPlayerSlot& dst = outPacket.slots[i];
        dst.sessionId = static_cast<uint32>(src.sessionId);
        dst.playerType = src.playerType;
        dst.ready = src.ready ? 1 : 0;
        dst.isHost = src.isHost ? 1 : 0;
        dst.rhythmR1SubVariant = static_cast<uint8>(src.rhythmVariantSelection.r1);
        dst.rhythmR2SubVariant = static_cast<uint8>(src.rhythmVariantSelection.r2);
        dst.rhythmR3SubVariant = static_cast<uint8>(src.rhythmVariantSelection.r3);
        if (src.isHost)
            outPacket.hostSlotIndex = static_cast<uint8>(i);
    }
}

void RoomNotifier::FillListEntry(const RoomState& room, RoomListEntry& out) const
{
    out.roomId = room.mRoomId;
    out.playerCount = static_cast<uint8>(room.mPlayers.size());
    out.maxPlayers = ROOM_MAX_PLAYERS;
    out.phase = static_cast<uint8>(room.mPhase);
    out.reserved = 0;
}

void RoomNotifier::FillRoomListPacket(S2C_RoomListPacket& outPacket) const
{
    uint8 count = 0;
    for (const auto& [roomId, room] : mRoomManager->GetRooms())
    {
        if (count >= ROOM_LIST_MAX_ENTRIES) break;
        FillListEntry(room, outPacket.entries[count]);
        ++count;
    }
    outPacket.count = count;
}

void RoomNotifier::BroadcastRoomState(uint32 roomId)
{
    RoomState* room = mRoomManager->GetRoom(roomId);
    if (room == nullptr) return;

    S2C_RoomStatePacket statePkt;
    FillStatePacket(*room, statePkt);

    // 브로드캐스트
    for (uint64 sessionId : room->GetSessionIds())
    {
        SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_STATE, sizeof(S2C_RoomStatePacket) };
        req.StoreAs<S2C_RoomStatePacket>(statePkt);
        gSendQueue.Push(req);
    }
}

void RoomNotifier::BroadcastRoomListToHall()
{
    S2C_RoomListPacket listPkt;
    FillRoomListPacket(listPkt);

    for (uint64 sessionId : mRoomManager->GetHallSessions())
    {
        SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_LIST, sizeof(S2C_RoomListPacket) };
        req.StoreAs<S2C_RoomListPacket>(listPkt);
        gSendQueue.Push(req);
    }
}

void RoomNotifier::SendRoomList(uint64 sessionId)
{
    S2C_RoomListPacket listPkt;
    FillRoomListPacket(listPkt);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_LIST, sizeof(S2C_RoomListPacket) };
    req.StoreAs<S2C_RoomListPacket>(listPkt);
    gSendQueue.Push(req);
}

void RoomNotifier::SendJoinResult(uint64 sessionId, uint32 roomId, bool success, RoomErrorCode code)
{
    S2C_RoomJoinResultPacket resultPkt;
    resultPkt.roomId = roomId;
    resultPkt.success = success ? 1 : 0;
    resultPkt.errorCode = static_cast<uint8>(code);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_JOIN_RESULT, sizeof(S2C_RoomJoinResultPacket) };
    req.StoreAs<S2C_RoomJoinResultPacket>(resultPkt);
    gSendQueue.Push(req);
}

void RoomNotifier::SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code)
{
    S2C_RoomErrorPacket errPkt;
    errPkt.roomId = roomId;
    errPkt.errorCode = static_cast<uint8>(code);

    SendRequest req{ static_cast<uint32>(sessionId), PKT_Type::S2C_ROOM_ERROR, sizeof(S2C_RoomErrorPacket) };
    req.StoreAs<S2C_RoomErrorPacket>(errPkt);
    gSendQueue.Push(req);
}
