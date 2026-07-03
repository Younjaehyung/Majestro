#include "pch.h"
#include "StressTest.h"
#include <algorithm>

static volatile bool sCtrlCPressed = false;

static BOOL WINAPI CtrlHandler(DWORD type)
{
	if (type == CTRL_C_EVENT) {
		sCtrlCPressed = true;
		return TRUE;
	}
	return FALSE;
}

StressTestManager::~StressTestManager()
{
	Shutdown();
}

bool StressTestManager::Initialize(const StressTestConfig& config)
{
	mConfig = config;

	// playersPerRoom 범위 보정
	if (mConfig.playersPerRoom < 1) mConfig.playersPerRoom = 1;
	if (mConfig.playersPerRoom > ROOM_MAX_PLAYERS) mConfig.playersPerRoom = ROOM_MAX_PLAYERS;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "[ERROR] WSAStartup 실패" << std::endl;
		return false;
	}

	// 클라이언트당 약 4개의 SendBuffer 사전 할당 (고갈 시 동적 확장)
	SendBufferManager::Initialize(config.totalClients * 4);

	// FullGame: 방 그룹 미리 구성
	mRoomGroups.clear();
	if (mConfig.scenario == TestScenario::FullGame) {
		int groupCount = (mConfig.totalClients + mConfig.playersPerRoom - 1) / mConfig.playersPerRoom;
		mRoomGroups.resize(groupCount);
		for (int g = 0; g < groupCount; ++g) {
			int remaining = mConfig.totalClients - g * mConfig.playersPerRoom;
			mRoomGroups[g].expectedSize = (std::min)(mConfig.playersPerRoom, remaining);
			mRoomGroups[g].serverRoomId = 0;
		}
	}

	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	sCtrlCPressed = false;

	return true;
}

void StressTestManager::PublishGroupRoomId(uint32 group, uint32 roomId)
{
	if (group < mRoomGroups.size())
		mRoomGroups[group].serverRoomId = roomId;
}

uint32 StressTestManager::GetGroupRoomId(uint32 group) const
{
	if (group < mRoomGroups.size())
		return mRoomGroups[group].serverRoomId;
	return 0;
}

void StressTestManager::Run()
{
	mRunning = true;
	auto now = Clock::now();
	mStartTime = now;
	mLastRampTime = now;
	mLastStatsPrint = now;
	mLastTick = now;

	const char* scenarioName =
		(mConfig.scenario == TestScenario::FullGame) ? "FullGame (로비→방→인게임)" : "LobbyChurn (방목록/방 churn)";

	std::cout << "========================================" << std::endl;
	std::cout << "  Stress Test Started" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "  Server:   " << mConfig.serverIp << ":" << mConfig.tcpPort << std::endl;
	std::cout << "  Scenario: " << scenarioName << std::endl;
	std::cout << "  Target:   " << mConfig.totalClients << " clients" << std::endl;
	std::cout << "  Ramp:     " << mConfig.rampUpPerSecond << " clients/sec" << std::endl;
	if (mConfig.scenario == TestScenario::FullGame) {
		std::cout << "  Room:     " << mConfig.playersPerRoom << " players/room ("
			<< mRoomGroups.size() << " rooms)" << std::endl;
		std::cout << "  Move:     " << (int)(1.0f / mConfig.moveSendInterval) << " Hz" << std::endl;
		std::cout << "  Action:   " << mConfig.actionInterval << " sec interval" << std::endl;
	}
	else {
		std::cout << "  Churn:    " << mConfig.churnInterval << " sec/cycle" << std::endl;
	}
	if (mConfig.testDuration > 0)
		std::cout << "  Duration: " << (int)mConfig.testDuration << " sec" << std::endl;
	else
		std::cout << "  Duration: Infinite" << std::endl;
	std::cout << "  Press Ctrl+C to stop" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << std::endl;

	while (mRunning && !sCtrlCPressed) {
		now = Clock::now();
		float dt = std::chrono::duration<float>(now - mLastTick).count();
		mLastTick = now;

		// dt 상한 - 스파이럴 방지
		if (dt > 0.1f) dt = 0.1f;

		// 더미 자체 루프 작업시간 계측(Sleep 제외). RTT 신뢰성 판단용.
		auto workStart = Clock::now();
		RampUp();
		NetworkTick();
		UpdateClients(dt);
		mLoopWorkAccumMs += std::chrono::duration<double, std::milli>(Clock::now() - workStart).count();
		++mLoopIterations;

		// 3초마다 통계 출력
		float statsDt = std::chrono::duration<float>(now - mLastStatsPrint).count();
		if (statsDt >= 3.0f) {
			CollectStats();
			PrintStats();
			mLastStatsPrint = now;
		}

		// 테스트 지속 시간 체크
		if (mConfig.testDuration > 0) {
			float elapsed = std::chrono::duration<float>(now - mStartTime).count();
			if (elapsed >= mConfig.testDuration) {
				std::cout << "\n[INFO] 테스트 시간 완료" << std::endl;
				break;
			}
		}

		Sleep(1); // CPU 양보
	}

	std::cout << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "  Stress Test Finished" << std::endl;
	std::cout << "========================================" << std::endl;
	CollectStats();
	PrintStats();
}

void StressTestManager::Shutdown()
{
	for (auto* client : mClients) {
		client->Disconnect();
		delete client;
	}
	mClients.clear();
	mRoomGroups.clear();
	mNextClientIndex = 0;
	mRampAccumulator = 0;
	SendBufferManager::Shutdown();
	WSACleanup();
	mRunning = false;
}

void StressTestManager::RampUp()
{
	if (mNextClientIndex >= mConfig.totalClients) return;

	auto now = Clock::now();
	float dt = std::chrono::duration<float>(now - mLastRampTime).count();
	mLastRampTime = now;

	mRampAccumulator += dt * mConfig.rampUpPerSecond;

	while (mRampAccumulator >= 1.0f && mNextClientIndex < mConfig.totalClients) {
		mRampAccumulator -= 1.0f;

		const int idx = mNextClientIndex;
		auto* client = new VirtualClient(idx);
		client->mMoveSendInterval = mConfig.moveSendInterval;
		client->mActionInterval = mConfig.actionInterval;
		client->mScenario = mConfig.scenario;
		client->mManager = this;

		if (mConfig.scenario == TestScenario::FullGame) {
			const int ppr = mConfig.playersPerRoom;
			const int group = idx / ppr;
			const int remaining = mConfig.totalClients - group * ppr;
			client->mRoomGroup = (uint32)group;
			client->mIsRoomHost = (idx % ppr == 0);
			client->mExpectedRoomSize = (uint8)(std::min)(ppr, remaining);
		}
		else {
			// LobbyChurn: 초기 churn 시점을 살짝 분산
			client->mChurnInterval = mConfig.churnInterval;
			client->mChurnTimer = (float)(idx % 10) * 0.1f;
		}

		if (client->StartConnect(mConfig.serverIp.c_str(), mConfig.tcpPort, mConfig.udpPort)) {
			mClients.push_back(client);
			mNextClientIndex++;
		}
		else {
			delete client;
			std::cout << "[ERROR] 클라이언트 " << mNextClientIndex << " 생성 실패" << std::endl;
			mNextClientIndex++;  // 무한 루프 방지
		}
	}
}

void StressTestManager::NetworkTick()
{
	if (mClients.empty()) return;

	fd_set readfds, writefds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	int socketCount = 0;

	for (auto* client : mClients) {
		if (!client->IsActive()) continue;

		SOCKET tcp = client->GetTcpSocket();
		SOCKET udp = client->GetUdpSocket();

		if (tcp == INVALID_SOCKET) continue;

		if (client->GetState() == ClientState::Connecting) {
			// 연결 중인 소켓은 writability 체크
			if (socketCount < FD_SETSIZE) {
				FD_SET(tcp, &writefds);
				socketCount++;
			}
		}
		else {
			// 연결된 소켓은 항상 readability 체크
			if (socketCount < FD_SETSIZE) {
				FD_SET(tcp, &readfds);
				socketCount++;
			}

			// 보낼 데이터가 있으면 writability 체크
			if (client->HasTcpDataToSend() && socketCount < FD_SETSIZE) {
				FD_SET(tcp, &writefds);
			}
		}

		if (udp != INVALID_SOCKET) {
			if (socketCount < FD_SETSIZE) {
				FD_SET(udp, &readfds);
				socketCount++;
			}

			if (client->HasUdpDataToSend() && socketCount < FD_SETSIZE) {
				FD_SET(udp, &writefds);
			}
		}
	}

	if (socketCount == 0) return;

	timeval timeout{};
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000; // 1ms

	int result = select(0, &readfds, &writefds, nullptr, &timeout);
	if (result <= 0) return;

	for (auto* client : mClients) {
		if (!client->IsActive()) continue;

		SOCKET tcp = client->GetTcpSocket();
		SOCKET udp = client->GetUdpSocket();

		if (tcp == INVALID_SOCKET) continue;

		if (client->GetState() == ClientState::Connecting) {
			if (FD_ISSET(tcp, &writefds)) {
				client->OnConnectComplete();
			}
			continue;
		}

		// TCP 수신
		if (FD_ISSET(tcp, &readfds)) {
			client->OnTcpReadable();
		}

		// TCP 전송
		if (FD_ISSET(tcp, &writefds)) {
			client->OnTcpWritable();
		}

		// UDP 수신
		if (udp != INVALID_SOCKET && FD_ISSET(udp, &readfds)) {
			client->OnUdpReadable();
		}

		// UDP 전송
		if (udp != INVALID_SOCKET && FD_ISSET(udp, &writefds)) {
			client->OnUdpWritable();
		}
	}
}

void StressTestManager::UpdateClients(float dt)
{
	for (auto* client : mClients) {
		if (client->IsActive()) {
			client->Update(dt);
		}
	}
}

void StressTestManager::CollectStats()
{
	mPrevStats = mStats;

	mStats = {};
	mStats.totalClients = (int)mClients.size();
	mStats.elapsedSeconds = std::chrono::duration<double>(Clock::now() - mStartTime).count();

	for (auto* client : mClients) {
		switch (client->GetState()) {
		case ClientState::Failed:
			mStats.failedClients++;
			break;
		case ClientState::Disconnected:
			break;
		case ClientState::Playing:
			mStats.playingClients++;
			mStats.connectedClients++;
			break;
		case ClientState::InRoom:
		case ClientState::Ready:
		case ClientState::SceneChanging:
		case ClientState::GameStarting:
			mStats.inRoomClients++;
			mStats.connectedClients++;
			break;
		case ClientState::InLobby:
		case ClientState::CreatingRoom:
		case ClientState::WaitingForRoom:
		case ClientState::JoiningRoom:
		case ClientState::Leaving:
		case ClientState::Registering:
			mStats.inLobbyClients++;
			mStats.connectedClients++;
			break;
		default:
			mStats.connectedClients++;
			break;
		}
		mStats.totalPacketsSent += client->mPacketsSent;
		mStats.totalPacketsRecv += client->mPacketsRecv;
		mStats.totalBytesSent += client->mBytesSent;
		mStats.totalBytesRecv += client->mBytesRecv;
	}

	// RTT 집계: 샘플이 있는 클라들의 최근 RTT 평균 + 전체 피크
	double rttSum = 0.0, rttMax = 0.0;
	int rttN = 0;
	for (auto* client : mClients) {
		if (client->mRttCount == 0) continue;
		rttSum += client->mLastRttMs;
		if (client->mRttMaxMs > rttMax) rttMax = client->mRttMaxMs;
		++rttN;
	}
	mStats.avgRttMs = (rttN > 0) ? (rttSum / rttN) : 0.0;
	mStats.maxRttMs = rttMax;
	mStats.rttSamples = rttN;

	// 서버 브로드캐스트 틱 집계: 누적 합 + 현재 관측 중인 클라 수
	for (auto* client : mClients) {
		mStats.totalSrvTicks += client->mSrvTickCount;
		if (client->mLastSrvSeq != 0) ++mStats.srvObservers;
	}
}

void StressTestManager::PrintStats()
{
	double dt = mStats.elapsedSeconds - mPrevStats.elapsedSeconds;
	if (dt <= 0) dt = 1.0;

	uint64 sendPps = (uint64)((mStats.totalPacketsSent - mPrevStats.totalPacketsSent) / dt);
	uint64 recvPps = (uint64)((mStats.totalPacketsRecv - mPrevStats.totalPacketsRecv) / dt);
	uint64 sendKBs = (uint64)((mStats.totalBytesSent - mPrevStats.totalBytesSent) / dt / 1024);
	uint64 recvKBs = (uint64)((mStats.totalBytesRecv - mPrevStats.totalBytesRecv) / dt / 1024);

	// 더미 루프 자체 부하: 평균 작업시간(ms) + 실효 루프 주파수(Hz)
	double loopAvgMs = (mLoopIterations > 0) ? (mLoopWorkAccumMs / mLoopIterations) : 0.0;
	double loopHz = (dt > 0) ? (mLoopIterations / dt) : 0.0;
	mLoopWorkAccumMs = 0;
	mLoopIterations = 0;

	// 서버 실제 브로드캐스트 Hz (관측 클라 1명 기준). 30 밑 = 서버가 못 따라옴.
	uint64 dSrvTicks = mStats.totalSrvTicks - mPrevStats.totalSrvTicks;
	double srvHz = (mStats.srvObservers > 0 && dt > 0) ? ((double)dSrvTicks / dt / mStats.srvObservers) : 0.0;

	printf("[%6.1fs] Conn %d/%d | Lobby:%d Room:%d Play:%d Fail:%d | RTT avg %.1f max %.1f ms (%d) | SrvHz %.0f | Loop %.1fms %.0fHz | Send %llu pps (%lluKB/s) | Recv %llu pps (%lluKB/s)\n",
		mStats.elapsedSeconds,
		mStats.connectedClients, mConfig.totalClients,
		mStats.inLobbyClients, mStats.inRoomClients, mStats.playingClients, mStats.failedClients,
		mStats.avgRttMs, mStats.maxRttMs, mStats.rttSamples,
		srvHz,
		loopAvgMs, loopHz,
		(unsigned long long)sendPps, (unsigned long long)sendKBs,
		(unsigned long long)recvPps, (unsigned long long)recvKBs);
}
