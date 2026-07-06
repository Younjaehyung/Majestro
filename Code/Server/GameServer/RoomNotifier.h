#pragma once
#include "PacketHelper.h"

class RoomManager;
class RoomState;

// Room 도메인 상태를 와이어 포맷(S2C_ROOM_*)으로 직렬화해 gSendQueue 로 송신하는 어댑터.
// RoomManager(도메인)는 패킷 구조를 모르고, 이 클래스를 통해서만 통지한다.
class RoomNotifier
{
public:
    void SetRoomManager(RoomManager* roomManager) { mRoomManager = roomManager; }

    void BroadcastRoomState(uint32 roomId);
    void BroadcastRoomListToHall();              // 홀 세션 전원에게 목록
    void SendRoomList(uint64 sessionId);         // 단일 세션에 목록
    void SendJoinResult(uint64 sessionId, uint32 roomId, bool success, RoomErrorCode code);
    void SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code);

private:
    void FillStatePacket(const RoomState& room, S2C_RoomStatePacket& outPacket) const;
    void FillListEntry(const RoomState& room, RoomListEntry& out) const;   // 방 목록용 요약
    void FillRoomListPacket(S2C_RoomListPacket& outPacket) const;

    RoomManager* mRoomManager = nullptr;
};
