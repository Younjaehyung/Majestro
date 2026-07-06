#pragma once
#include "PacketHelper.h"

class SceneManager;
class RoomPacketHandler;
class ScenePacketHandler;

// gRecvQueue 에서 꺼낸 패킷을 담당 핸들러로 분배하는 최상위 라우터.
// SceneManager/RoomManager 는 서로의 패킷을 모르고, 전체 분배 규칙은 여기에만 있다.
class PacketRouter
{
public:
    PacketRouter(SceneManager* sceneManager, RoomPacketHandler* roomHandler, ScenePacketHandler* sceneHandler)
        : mSceneManager(sceneManager), mRoomHandler(roomHandler), mSceneHandler(sceneHandler) {}

    bool Dispatch(const InputCommand& command);

private:
    static bool IsRoomPacket(PKT_Type type);

    SceneManager* mSceneManager = nullptr;
    RoomPacketHandler* mRoomHandler = nullptr;
    ScenePacketHandler* mSceneHandler = nullptr;
};
