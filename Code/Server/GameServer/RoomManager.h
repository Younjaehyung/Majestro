#pragma once
#include "PacketHelper.h"


// 방 진행 단계: 대기실(입장 허용) / 게임 중(난입 금지)
enum class RoomPhase : uint8 { Waiting = 0, InGame = 1 };

struct RoomPlayerEntry
{
    uint64 sessionId = 0;
    uint8  playerType = 0;
    bool   ready = false;
    bool   isHost = false;  // 첫 입장자가 Host.
};

class RoomState
{   // RoomManager 가 RoomState(들) 을 소유
public:
    uint32 mRoomId = 0;                       // 0 = 무효. CreateRoom 이 발급
    RoomPhase mPhase = RoomPhase::Waiting;    // 대기실 / 게임 중
    std::vector<RoomPlayerEntry> mPlayers;
    uint64 mHostSessionId = 0;

    // 같은 sessionId 가 이미 있으면 false. 빈 방이었으면 호스트로 등록.
    bool AddPlayer(uint64 sessionId);

    // 호스트가 빠지면 남은 첫 슬롯에게 호스트 승계. 마지막 한 명이 빠지면 호스트 0.
    bool RemovePlayer(uint64 sessionId);

    bool SetReady(uint64 sessionId, bool ready);

    // 캐릭터 변경 시 변경한 본인의 ready 를 false 로 자동 해제
    bool SetPlayerCharacter(uint64 sessionId, uint8 playerType);

    bool IsAllReady() const;
    bool IsHost(uint64 sessionId) const;

    void FillStatePacket(S2C_RoomStatePacket& outPacket) const;
    void FillListEntry(RoomListEntry& out) const;   // 방 목록용 요약
    std::vector<uint64> GetSessionIds() const;
};

class RoomManager
{
    // 접속 직후 세션은 어떤 방에도 속하지 않는 mHallSessions 상태.
public:
    void Initialize();

    // 세션 생명주기
    void OnSessionEnterLobby(uint64 sessionId);  // 자동입장 제거: 홀 등록 + 방 목록 송신
    void OnSessionLeave(uint64 sessionId);       // 접속 종료: 방에서 빼고 빈 방 소멸

    bool HandleRoomPacket(const InputCommand& command);

    // 실패 시 outError 에 거부 사유 반환
    bool CanStartGame(uint64 sessionId, RoomErrorCode& outError) const;

    // 게임 시작이 승인됐을 때 호출. phase=InGame + 목록 갱신
    void OnGameStarted(uint64 sessionId);

    RoomState* GetRoomByPlayer(uint64 sessionId);
    RoomState* GetRoom(uint32 roomId);
    uint32     GetRoomIdByPlayer(uint64 sessionId) const;  // 없으면 0

private:
    // C2S_ROOM_* 핸들러
    void HandleCreate(uint64 sessionId);
    void HandleJoin(uint64 sessionId, uint32 roomId);
    void HandleLeave(uint64 sessionId);
    void HandleList(uint64 sessionId);

    void BroadcastRoomState(uint32 roomId);
    void BroadcastRoomListToHall();              // 홀 세션 전원에게 목록
    void SendRoomList(uint64 sessionId);         // 단일 세션에 목록
    void FillRoomListPacket(S2C_RoomListPacket& outPacket) const;
    void SendJoinResult(uint64 sessionId, uint32 roomId, bool success, RoomErrorCode code);
    void SendError(uint64 sessionId, uint32 roomId, RoomErrorCode code);

    std::unordered_map<uint32, RoomState> mRooms;          // roomId - RoomState
    std::unordered_map<uint64, uint32>    mSessionToRoom;  // sessionId - roomId (방에 속한 세션만)
    std::unordered_set<uint64>            mHallSessions;    // 접속시 방 리스트창 상태
    uint32 mNextRoomId = 1;                                // roomId 발급 카운터 (재사용 안 함)

    static constexpr uint8 mMinPlayersToStart = 1; // 혼자도 시작 허용
};
