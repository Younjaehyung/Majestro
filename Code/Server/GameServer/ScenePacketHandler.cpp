#include "pch.h"
#include "ScenePacketHandler.h"
#include "SceneManager.h"
#include "ServerCore.h"

bool ScenePacketHandler::Handle(const InputCommand& command)
{
    const C2S_SceneChangePacket* requestPacket = command.ViewAs<C2S_SceneChangePacket>();
    if (!requestPacket)
        return false;

    const SceneChangeOutcome outcome = mSceneManager->TryChangeScene(command.SessionId, requestPacket->targetScene);
    if (!outcome.sendResponse)   // 디버그 강제 전환 등 응답 불필요
        return true;

    // 요청자에게 결과 통지
    S2C_SceneChangeResultPacket responsePacket(outcome.resultScene, outcome.approved);
    SendRequest response{ static_cast<uint32>(command.SessionId), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(S2C_SceneChangeResultPacket) };
    response.StoreAs<S2C_SceneChangeResultPacket>(responsePacket);
    gSendQueue.Push(response);

    // 로비 게임 시작 자격 거부 사유 통지
    if (outcome.roomError != RoomErrorCode::None)
    {
        S2C_RoomErrorPacket errPkt;
        errPkt.roomId = outcome.errorRoomId;
        errPkt.errorCode = static_cast<uint8>(outcome.roomError);
        SendRequest errReq{ static_cast<uint32>(command.SessionId), PKT_Type::S2C_ROOM_ERROR, sizeof(errPkt) };
        errReq.StoreAs<S2C_RoomErrorPacket>(errPkt);
        gSendQueue.Push(errReq);
    }

    // 함께 전환되는 나머지 방원에게 동일한 승인 패킷 전송
    for (uint64 otherSessionId : outcome.alsoNotifySessions)
    {
        S2C_SceneChangeResultPacket otherPkt(outcome.resultScene, true);
        SendRequest otherReq{ static_cast<uint32>(otherSessionId), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(otherPkt) };
        otherReq.StoreAs<S2C_SceneChangeResultPacket>(otherPkt);
        gSendQueue.Push(otherReq);
    }

    return true;
}
