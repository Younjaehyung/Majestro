#include "pch.h"
#include "StressTest.h"

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

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "[ERROR] WSAStartup 실패" << std::endl;
		return false;
	}

	// 클라이언트당 약 4개의 SendBuffer 사전 할당
	SendBufferManager::Initialize(config.totalClients * 4);

	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	sCtrlCPressed = false;

	return true;
}

void StressTestManager::Run()
{
	mRunning = true;
	auto now = Clock::now();
	mStartTime = now;
	mLastRampTime = now;
	mLastStatsPrint = now;
	mLastTick = now;

	std::cout << "========================================" << std::endl;
	std::cout << "  Stress Test Started" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "  Server: " << mConfig.serverIp << ":" << mConfig.tcpPort << std::endl;
	std::cout << "  Target: " << mConfig.totalClients << " clients" << std::endl;
	std::cout << "  Ramp:   " << mConfig.rampUpPerSecond << " clients/sec" << std::endl;
	std::cout << "  Move:   " << (int)(1.0f / mConfig.moveSendInterval) << " Hz" << std::endl;
	std::cout << "  Action: " << mConfig.actionInterval << " sec interval" << std::endl;
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

		RampUp();
		NetworkTick();
		UpdateClients(dt);

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

		auto* client = new VirtualClient(mNextClientIndex);
		client->mMoveSendInterval = mConfig.moveSendInterval;
		client->mActionInterval = mConfig.actionInterval;

		if (client->StartConnect(mConfig.serverIp.c_str(), mConfig.tcpPort, mConfig.udpPort)) {
			mClients.push_back(client);
			mNextClientIndex++;
		}
		else {
			delete client;
			std::cout << "[ERROR] 클라이언트 " << mNextClientIndex << " 생성 실패" << std::endl;
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
		case ClientState::Playing:
			mStats.playingClients++;
			mStats.connectedClients++;
			break;
		case ClientState::Disconnected:
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
}

void StressTestManager::PrintStats()
{
	double dt = mStats.elapsedSeconds - mPrevStats.elapsedSeconds;
	if (dt <= 0) dt = 1.0;

	uint64 sendPps = (uint64)((mStats.totalPacketsSent - mPrevStats.totalPacketsSent) / dt);
	uint64 recvPps = (uint64)((mStats.totalPacketsRecv - mPrevStats.totalPacketsRecv) / dt);
	uint64 sendKBs = (uint64)((mStats.totalBytesSent - mPrevStats.totalBytesSent) / dt / 1024);
	uint64 recvKBs = (uint64)((mStats.totalBytesRecv - mPrevStats.totalBytesRecv) / dt / 1024);

	printf("[%6.1fs] Clients: %d/%d (Playing: %d, Failed: %d) | Send: %llu pps (%lluKB/s) | Recv: %llu pps (%lluKB/s)\n",
		mStats.elapsedSeconds,
		mStats.connectedClients, mConfig.totalClients,
		mStats.playingClients, mStats.failedClients,
		(unsigned long long)sendPps, (unsigned long long)sendKBs,
		(unsigned long long)recvPps, (unsigned long long)recvKBs);
}
