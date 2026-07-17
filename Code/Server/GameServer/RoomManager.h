#pragma once
#include "PacketHelper.h"

class RoomNotifier;

// 방 진행 단계: 대기실(입장 허용) / 게임 중(난입 금지)
enum class RoomPhase : uint8 { Waiting = 0, InGame = 1 };

struct RoomPlayerEntry
{
    uint64 sessionId = 0;
    uint8  playerType = 0;
    RhythmMusicVariant rhythmMusicSubVariant = RhythmMusicVariant::Original;
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
    uint8  mNextStageIndex = 0;               // kStageOrder 인덱스. 다음에 도전할 스테이지 (레벨 클리어 시 증가)

    // 같은 sessionId 가 이미 있으면 false. 빈 방이었으면 호스트로 등록.
    bool AddPlayer(uint64 sessionId);

    // 호스트가 빠지면 남은 첫 슬롯에게 호스트 승계. 마지막 한 명이 빠지면 호스트 0.
    bool RemovePlayer(uint64 sessionId);

    bool SetReady(uint64 sessionId, bool ready);
    bool SetRhythmMusicSelection(uint64 sessionId, RhythmMusicVariant selection);

    // 캐릭터 변경 시 변경한 본인의 ready 를 false 로 자동 해제
    bool SetPlayerCharacter(uint64 sessionId, uint8 playerType);
    bool IsCharacterLockedByOtherReadyPlayer(uint64 sessionId, uint8 playerType) const;

    // 세션의 선택 캐릭터 조회
    bool GetPlayerType(uint64 sessionId, uint8& outType) const;

    // 한 캐릭터를 확정한 직후, 같은 캐릭터를 고르던 미확정 플레이어들을 자동 이동
    void EvictConflictingSelections(uint64 lockerSessionId, uint8 lockedType);

    // 해당 세션이 쓸 수 있는 캐릭터
    uint8 PickFreeCharacter(uint64 sessionId) const;

    bool IsAllReady() const;
    bool IsHost(uint64 sessionId) const;

    std::vector<uint64> GetSessionIds() const;
};

// 방 도메인 서비스. 패킷 파싱/송신은 RoomPacketHandler/RoomNotifier 담당이며,
// 여기서는 상태 변경과 규칙 검사만 수행한다. (상태 변경에 따른 통지는 mNotifier 경유)
class RoomManager
{
    // 접속 직후 세션은 어떤 방에도 속하지 않는 mHallSessions 상태.
public:
    void Initialize();
    void SetNotifier(RoomNotifier* notifier) { mNotifier = notifier; }

    // 세션 생명주기 (상태 변경 + 통지까지 수행)
    void OnSessionEnterLobby(uint64 sessionId);  // 자동입장 제거: 홀 등록 + 방 목록 송신
    void OnSessionLeave(uint64 sessionId);       // 접속 종료: 방에서 빼고 빈 방 소멸

    // 게임 시작이 승인됐을 때 호출. phase=InGame + 목록 갱신
    void OnGameStarted(uint64 sessionId);

    // 도메인 조작 — 규칙 검사 후 상태만 변경. 응답/브로드캐스트는 호출자(핸들러) 책임.
    RoomErrorCode CreateRoom(uint64 sessionId, uint32& outRoomId);
    RoomErrorCode JoinRoom(uint64 sessionId, uint32 roomId);
    bool LeaveRoom(uint64 sessionId, uint32& outRoomId);   // true = 방에 속해 있었고 나감
    RoomErrorCode SetPlayerReady(uint64 sessionId, bool ready);
    RoomErrorCode SelectRhythmMusic(uint64 sessionId, RhythmMusicVariant selection);
    RoomErrorCode SelectCharacter(uint64 sessionId, uint8 playerType);

    // 실패 시 outError 에 거부 사유 반환
    bool CanStartGame(uint64 sessionId, RoomErrorCode& outError) const;

    RoomState* GetRoomByPlayer(uint64 sessionId);
    RoomState* GetRoom(uint32 roomId);
    uint32     GetRoomIdByPlayer(uint64 sessionId) const;  // 없으면 0

    // Notifier 직렬화용 읽기 전용 접근
    const std::unordered_map<uint32, RoomState>& GetRooms() const { return mRooms; }
    const std::unordered_set<uint64>& GetHallSessions() const { return mHallSessions; }

private:
    std::unordered_map<uint32, RoomState> mRooms;          // roomId - RoomState
    std::unordered_map<uint64, uint32>    mSessionToRoom;  // sessionId - roomId (방에 속한 세션만)
    std::unordered_set<uint64>            mHallSessions;    // 접속시 방 리스트창 상태
    uint32 mNextRoomId = 1;                                // roomId 발급 카운터 (재사용 안 함)

    RoomNotifier* mNotifier = nullptr;

    static constexpr uint8 mMinPlayersToStart = 1; // 혼자도 시작 허용
};
