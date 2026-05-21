#pragma once
#include "PacketHelper.h"
// 서버 전역 단일 RoomManager 가 RoomState(들) 을 소유.
// v1 에서는 roomId = 1 인 RoomState 한 개만 존재. 첫 입장자가 Host.
// 게임 월드는 여전히 SceneManager::mActiveScene 하나를 공유한다.
// RoomState 와 Scene 은 직접 연결되지 않는다 (추후 다방 확장 시 roomId 과 Scene 매핑을 추가).
// 모든 외부 API 와 패킷에는 roomId 가 포함된다 (현재는 1 고정).

struct RoomPlayerEntry
{
    uint64 sessionId = 0;
    uint8  playerType = 0;
    bool   ready = false;
    bool   isHost = false;
};

class RoomState
{
public:
    uint32 mRoomId = 1;
    std::vector<RoomPlayerEntry> mPlayers; 
    uint64 mHostSessionId = 0;

    // 같은 sessionId 가 이미 있으면 false. 빈 방이었으면 호스트로 등록.
    bool AddPlayer(uint64 sessionId);

    // 호스트가 빠지면 남은 첫 슬롯에게 호스트 승계. 마지막 한 명이 빠지면 호스트 0.
    bool RemovePlayer(uint64 sessionId);

    bool SetReady(uint64 sessionId, bool ready);

    // 캐릭터 변경 시 변경한 본인의 ready 를 false 로 자동 해제
    bool SetCharacter(uint64 sessionId, uint8 playerType);

    // 게임 시작 직후 모든 슬롯 ready 를 false 로 일괄 리셋. 세션 자체는 유지.
    void ResetAllReady();

    bool IsAllReady() const;
    bool IsHost(uint64 sessionId) const;

    void FillStatePacket(S2C_RoomStatePacket& outPacket) const;
    std::vector<uint64> GetSessionIds() const;
};

class RoomManager
{
public:
    void Initialize();

    // SceneManager 후크 - 세션이 로비에 들어오는 순간 / 빠지는 순간
    void OnSessionEnterLobby(uint64 sessionId);
    void OnSessionLeave(uint64 sessionId);

    // SceneManager::EnqueueCommand 에서 C2S_ROOM_* 패킷을 라우팅
    bool HandleRoomPacket(const InputCommand& command);

    // SceneManager::HandleSceneChange 의 Lobby에서 시작시호출.
    // 실패 시 outError 에 거부 사유 반환.
    bool CanStartGame(uint64 sessionId, RoomErrorCode& outError) const;

    // 게임 시작이 승인됐을 때 호출. ready 일괄 리셋 후 상태 브로드캐스트.
    void OnGameStarted(uint64 sessionId);

    RoomState* GetRoomByPlayer(uint64 sessionId);
    RoomState* GetRoom(uint32 roomId);

private:
    void BroadcastRoomState(uint32 roomId);
    void SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code);

    std::unordered_map<uint32, RoomState> mRooms;          // roomId - RoomState
    std::unordered_map<uint64, uint32>    mSessionToRoom;  // sessionId - roomId

    static constexpr uint8 mMinPlayersToStart = 1; // 혼자도 시작 허용 (임시)
    static constexpr uint32 mDefaultRoomId = 1;    // 단일 방 (임시)
};
