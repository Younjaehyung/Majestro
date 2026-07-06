#include "pch.h"
#include "PacketRouter.h"
#include "SceneManager.h"
#include "RoomPacketHandler.h"
#include "ScenePacketHandler.h"

bool PacketRouter::IsRoomPacket(PKT_Type type)
{
    switch (type)
    {
    case PKT_Type::C2S_ROOM_READY:
    case PKT_Type::C2S_ROOM_CHARACTER_SELECT:
    case PKT_Type::C2S_ROOM_CREATE:
    case PKT_Type::C2S_ROOM_JOIN:
    case PKT_Type::C2S_ROOM_LIST:
    case PKT_Type::C2S_ROOM_LEAVE:
        return true;
    default:
        return false;
    }
}

bool PacketRouter::Dispatch(const InputCommand& command)
{
    if (command.Type == PKT_Type::INTERNAL_SESSION_LEAVE)	// 네트워크 스레드가 보낸 세션 종료 통지
    {
        mSceneManager->RemoveSession(command.SessionId);	// 방/씬/엔티티 정리
        return true;
    }

    // Room 패킷은 World 가 아니라 RoomPacketHandler 가 처리
    if (IsRoomPacket(command.Type))
        return mRoomHandler ? mRoomHandler->Handle(command) : false;

    if (command.Type == PKT_Type::C2S_SCENE_CHANGE)
        return mSceneHandler ? mSceneHandler->Handle(command) : false;

    // 나머지 게임플레이 입력은 세션이 속한 World 의 입력 큐로
    return mSceneManager->EnqueueToWorld(command);
}
