#pragma once
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include <queue>
#include <chrono>

class StressTestManager;

// 스트레스 테스트 시나리오
//  - LobbyChurn : 로비(방목록) + 방 생성/입장/레디/퇴장을 반복 → RoomManager/브로드캐스트 부하
//  - FullGame   : 방 구성 → 전원 Ready → Host 게임 시작 → 인게임 이동/액션 → (반복 옵션)
enum class TestScenario : uint8 {
	LobbyChurn = 0,
	FullGame = 1,
};

enum class ClientState : uint8 {
	Disconnected,
	Connecting,      // Non-blocking TCP connect in progress
	TcpConnected,    // TCP connected, waiting for S2C_LoginPacket
	Registering,     // UDP 로그인 등록(세션→홀 진입) 후 로비로
	InLobby,         // 홀(방 목록) 상태. LobbyChurn 의 churn 시작점
	CreatingRoom,    // C2S_ROOM_CREATE 송신, JoinResult 대기 (Host)
	WaitingForRoom,  // Host 가 roomId 발급할 때까지 대기 (Member)
	JoiningRoom,     // C2S_ROOM_JOIN 송신, JoinResult 대기 (Member)
	InRoom,          // 방 입장 완료, 아직 Ready 아님
	Ready,           // Ready 완료. Host=전원Ready 대기, Member=씬전환 대기
	SceneChanging,   // 게임 시작(씬 전환) 요청/응답 대기
	GameStarting,    // C2S_GAME_START 송신, 스폰 대기
	Playing,         // 인게임 (이동/액션 시뮬레이션)
	Leaving,         // LobbyChurn: C2S_ROOM_LEAVE 송신 후 홀 복귀 대기
	Failed           // 오류 상태
};

const char* ClientStateToString(ClientState state);

class VirtualClient {
public:
	VirtualClient(uint32 id);
	~VirtualClient();

	// Connection
	bool StartConnect(const char* ip, int tcpPort, int udpPort);
	void OnConnectComplete();
	void Disconnect();

	// Network I/O (called by StressTestManager)
	void OnTcpReadable();
	void OnUdpReadable();
	bool HasTcpDataToSend() const { return !mTcpSendQueue.empty(); }
	bool HasUdpDataToSend() const { return !mUdpSendQueue.empty(); }
	void OnTcpWritable();
	void OnUdpWritable();

	// State update
	void Update(float dt);

	// Getters
	uint32 GetId() const { return mId; }
	uint32 GetClientId() const { return mClientId; }
	ClientState GetState() const { return mState; }
	SOCKET GetTcpSocket() const { return mTcpSocket; }
	SOCKET GetUdpSocket() const { return mUdpSocket; }
	bool IsActive() const { return mState != ClientState::Disconnected && mState != ClientState::Failed; }

	// Stats
	uint64 mPacketsSent = 0;
	uint64 mPacketsRecv = 0;
	uint64 mBytesSent = 0;
	uint64 mBytesRecv = 0;

	// RTT (인게임 Sync echo 왕복시간, ms)
	double mLastRttMs = 0;   // 최근 1회 RTT
	double mRttMaxMs = 0;    // 관측된 최대 RTT
	double mRttSumMs = 0;    // 누적 합(평균 계산용)
	uint64 mRttCount = 0;    // 샘플 수

	// 서버 브로드캐스트 틱 관측: S2C_PKT_MOVE 의 Sequence(=서버 sendTick) 전진 횟수.
	// 초당 카운트 = 서버 실제 브로드캐스트 Hz. 30 밑으로 떨어지면 서버가 못 따라옴.
	uint32 mLastSrvSeq = 0;
	uint64 mSrvTickCount = 0;

private:
	void ProcessTcpPacket(BYTE* buffer, uint32 size);
	void ProcessUdpPacket(BYTE* buffer, uint32 size);
	void SendTcp(const void* data, uint32 size);
	void SendUdp(const void* data, uint32 size);
	void SimulateGameplay(float dt);
	void SetState(ClientState newState);

	// Room / lobby flow
	void OnRegistered();                 // 로그인 완료 후 시나리오 분기
	void TryStartRoomFlow();             // FullGame: Host=Create / Member=대기
	void OnRoomState(const struct S2C_RoomStatePacket& pkt);
	void OnJoinResult(const struct S2C_RoomJoinResultPacket& pkt);
	void OnSceneChangeResult(const struct S2C_SceneChangeResultPacket& pkt);
	void SendRoomReady();
	void SendSceneChangeToGame();
	void SendGameStart();
	void UpdateLobbyChurn(float dt);     // LobbyChurn 시나리오 진행

	uint32 mId;                    // Local index
	uint32 mClientId = 0;         // Server-assigned ID
	ClientState mState = ClientState::Disconnected;

	SOCKET mTcpSocket = INVALID_SOCKET;
	SOCKET mUdpSocket = INVALID_SOCKET;
	sockaddr_in mServerTcpAddr{};
	sockaddr_in mServerUdpAddr{};

	RecvBuffer mTcpRecvBuffer;
	BYTE mUdpRecvBuffer[4096]{};
	std::queue<SendBuffer*> mTcpSendQueue;
	std::queue<SendBuffer*> mUdpSendQueue;

	// Simulation state
	uint64 mNetEntityId = 0;       // 서버가 부여한 네트워크 ID(본인 스폰에서 캡처)
	float mPosX = 0, mPosY = 0, mPosZ = 0;
	float mYaw = 0;
	float mMoveTimer = 0;
	float mActionTimer = 0;
	float mSyncTimer = 0;        // RTT 측정용 Sync 송신 간격
	float mDirectionTimer = 0;
	float mMoveX = 0, mMoveZ = 0;
	uint32 mMoveSeq = 0;

	// Timing
	using Clock = std::chrono::steady_clock;
	Clock::time_point mStateChangeTime;

	// Config (set by StressTestManager)
	float mMoveSendInterval = 0.033f;  // 30 Hz
	float mActionInterval = 2.0f;
	TestScenario mScenario = TestScenario::FullGame;

	// Room coordination (set by StressTestManager)
	StressTestManager* mManager = nullptr;
	uint32 mRoomGroup = 0;        // 같은 그룹끼리 한 방
	bool   mIsRoomHost = false;   // 그룹의 첫 클라이언트
	uint8  mExpectedRoomSize = 1; // 이 방에 모일 예상 인원

	// Room runtime
	uint32 mRoomId = 0;           // 서버 발급 roomId (0 = 미발급)
	uint8  mRoomPlayerType = 0;   // 방에서 배정받은 캐릭터
	bool   mReadySent = false;
	bool   mRoomAllReady = false; // (Host) 방 전원 Ready 도달 여부
	float  mRoomActionTimer = 0;  // 상태 진행 재시도/타임아웃 가드

	// LobbyChurn 진행 타이머
	float  mChurnInterval = 1.0f; // 방에 머무는 시간(매니저가 설정)
	float  mChurnTimer = 0;
	int    mChurnCycle = 0;

	friend class StressTestManager;
};
