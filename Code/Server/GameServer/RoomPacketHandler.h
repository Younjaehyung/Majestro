#pragma once
#include "PacketHelper.h"

class RoomManager;
class RoomNotifier;

// C2S_ROOM_* 패킷 파싱 → RoomManager(도메인) 호출 → 응답/브로드캐스트 송신.
// 도메인 규칙은 RoomManager/RoomState 에 있고, 여기서는 검증 실패·성공에 따른 통지만 담당한다.
class RoomPacketHandler
{
public:
    RoomPacketHandler(RoomManager* roomManager, RoomNotifier* notifier)
        : mRoomManager(roomManager), mNotifier(notifier) {}

    bool Handle(const InputCommand& command);

private:
    void HandleCreate(uint64 sessionId);
    void HandleJoin(uint64 sessionId, uint32 roomId);
    void HandleLeave(uint64 sessionId);
    void HandleList(uint64 sessionId);
    bool HandleReady(const InputCommand& command);
    bool HandleCharacterSelect(const InputCommand& command);

    RoomManager* mRoomManager = nullptr;
    RoomNotifier* mNotifier = nullptr;
};
