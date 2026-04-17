#pragma once
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include <queue>
#include <chrono>

enum class ClientState : uint8 {
	Disconnected,
	Connecting,      // Non-blocking TCP connect in progress
	TcpConnected,    // TCP connected, waiting for S2C_LoginPacket
	LoggedIn,        // Login done, UDP registration + scene change
	SceneChanging,   // Scene change requested, waiting response
	GameStarting,    // Game start requested, waiting response
	Playing,         // In-game (move/action simulation)
	Failed           // Error state
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

private:
	void ProcessTcpPacket(BYTE* buffer, uint32 size);
	void ProcessUdpPacket(BYTE* buffer, uint32 size);
	void SendTcp(const void* data, uint32 size);
	void SendUdp(const void* data, uint32 size);
	void SimulateGameplay(float dt);
	void SetState(ClientState newState);

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
	float mPosX = 0, mPosY = 0, mPosZ = 0;
	float mYaw = 0;
	float mMoveTimer = 0;
	float mActionTimer = 0;
	float mDirectionTimer = 0;
	float mMoveX = 0, mMoveZ = 0;
	uint32 mMoveSeq = 0;

	// Timing
	using Clock = std::chrono::steady_clock;
	Clock::time_point mStateChangeTime;

	// Config (set by StressTestManager)
	float mMoveSendInterval = 0.033f;  // 30 Hz
	float mActionInterval = 2.0f;

	friend class StressTestManager;
};
