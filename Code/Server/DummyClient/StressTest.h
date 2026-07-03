#pragma once
#include "VirtualClient.h"
#include <vector>
#include <string>
#include <chrono>

struct StressTestConfig {
	std::string serverIp = "127.0.0.1";
	int tcpPort = 9000;
	int udpPort = 9001;
	int totalClients = 10;
	int rampUpPerSecond = 5;
	float moveSendInterval = 0.033f;   // seconds (30 Hz)
	float actionInterval = 2.0f;       // seconds
	float testDuration = 0.0f;         // 0 = infinite

	// 시나리오: LobbyChurn(로비/방목록 부하) / FullGame(방 구성→인게임)
	TestScenario scenario = TestScenario::FullGame;
	// FullGame: 한 방에 모을 인원 (1~ROOM_MAX_PLAYERS)
	int playersPerRoom = ROOM_MAX_PLAYERS;
	// LobbyChurn: 한 사이클(생성→레디→퇴장) 간격(초)
	float churnInterval = 1.0f;
};

struct StressTestStats {
	int totalClients = 0;
	int connectedClients = 0;
	int inLobbyClients = 0;   // 홀/방목록 단계
	int inRoomClients = 0;    // 방 안(레디 포함, 게임 전)
	int playingClients = 0;   // 인게임
	int failedClients = 0;
	uint64 totalPacketsSent = 0;
	uint64 totalPacketsRecv = 0;
	uint64 totalBytesSent = 0;
	uint64 totalBytesRecv = 0;
	double elapsedSeconds = 0;

	// RTT (인게임 Sync 왕복, ms)
	double avgRttMs = 0;   // 현재 RTT 샘플 있는 클라들의 평균(최근값)
	double maxRttMs = 0;   // 관측된 최대 RTT(피크)
	int    rttSamples = 0; // RTT 샘플을 가진 클라 수

	// 서버 브로드캐스트 Hz 관측(서버가 30Hz를 유지하는지)
	uint64 totalSrvTicks = 0; // 관측된 서버 브로드캐스트 틱 누적 합
	int    srvObservers = 0;  // 브로드캐스트를 받고 있는 클라 수
};

// 그룹(=방) 단위 코디네이션. Host 가 서버 roomId 를 발급받으면 여기에 publish,
// Member 들은 자기 그룹의 roomId 가 채워질 때까지 대기 후 join.
struct RoomGroup {
	uint32 serverRoomId = 0;  // 0 = Host 가 아직 방을 못 만듦
	int    expectedSize = 0;  // 이 방에 모일 예상 인원
};

class StressTestManager {
public:
	StressTestManager() = default;
	~StressTestManager();

	bool Initialize(const StressTestConfig& config);
	void Run();
	void Shutdown();

	// VirtualClient 가 호출하는 방 코디네이션 API
	void PublishGroupRoomId(uint32 group, uint32 roomId);
	uint32 GetGroupRoomId(uint32 group) const;

private:
	void RampUp();
	void NetworkTick();
	void UpdateClients(float dt);
	void CollectStats();
	void PrintStats();

	StressTestConfig mConfig;
	StressTestStats mStats{};
	StressTestStats mPrevStats{}; // For PPS calculation
	std::vector<VirtualClient*> mClients;
	std::vector<RoomGroup> mRoomGroups;  // group index → 방 정보

	using Clock = std::chrono::steady_clock;
	Clock::time_point mStartTime;
	Clock::time_point mLastRampTime;
	Clock::time_point mLastStatsPrint;
	Clock::time_point mLastTick;

	int mNextClientIndex = 0;
	float mRampAccumulator = 0;
	bool mRunning = false;

	// 더미 자체 루프 부하 계측 (RTT 신뢰성 판단용)
	double mLoopWorkAccumMs = 0;  // 통계 창 동안 누적된 루프 작업시간(ms)
	uint64 mLoopIterations = 0;   // 통계 창 동안 루프 반복수
};
