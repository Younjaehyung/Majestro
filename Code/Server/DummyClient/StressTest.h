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
};

struct StressTestStats {
	int totalClients = 0;
	int connectedClients = 0;
	int playingClients = 0;
	int failedClients = 0;
	uint64 totalPacketsSent = 0;
	uint64 totalPacketsRecv = 0;
	uint64 totalBytesSent = 0;
	uint64 totalBytesRecv = 0;
	double elapsedSeconds = 0;
};

class StressTestManager {
public:
	StressTestManager() = default;
	~StressTestManager();

	bool Initialize(const StressTestConfig& config);
	void Run();
	void Shutdown();

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

	using Clock = std::chrono::steady_clock;
	Clock::time_point mStartTime;
	Clock::time_point mLastRampTime;
	Clock::time_point mLastStatsPrint;
	Clock::time_point mLastTick;

	int mNextClientIndex = 0;
	float mRampAccumulator = 0;
	bool mRunning = false;
};
