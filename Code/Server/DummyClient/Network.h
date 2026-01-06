#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include "Packet.h"
#include "RingBuffer.h"
#include "PacketHelper.h"

constexpr const char* SERVERIP = "127.0.0.1";
constexpr int SERVERPORT = 9000;
constexpr int BUFSIZE = 4096;

struct PendingSend {
	BYTE* data;
	int size;
	int sentBytes;
};



class Network
{
public:
	uint32_t mClientId{};
private:
	WSADATA mWsaData{};
	SOCKET mSock{};
	sockaddr_in mServerAddr{};
	
private:
	std::thread mNetworkThread;
	RecvBuffer mRecvBuffer;
	std::queue<SendBuffer*> mSendBuffer;
	std::mutex mQueueMutex;
	std::atomic<bool> mIsRunning;


	Network();
	~Network();
public: // Init
	static Network& GetInstance() {
		static Network instance;
		return instance;
	}


	void Initialize();
	void ConnectToServer(const char* ipAddress = SERVERIP, int port = SERVERPORT);
	void ReleaseServer();

	void Awake();
	void CheckConnect();

	int32 Onrecv(BYTE* buffer, int32 len);

public: // Process

	void NetworkUpdate();
	void GameRecvUpdate();
	void GameSendUpdate();

	void ProcessPacket(BYTE* buffer, int32 len);
	void PushSendData(const uint8_t* data, size_t size);

};

