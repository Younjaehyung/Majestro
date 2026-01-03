#pragma once
#include "pch.h"
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include "Packet.h"
#include "RingBuffer.h"
#include "PacketHelper.h"

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크


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
	std::thread mNetworkThread;

	const char* SERVERIP = "127.0.0.1";
	char mRecvBuf[BUFSIZ] = {};
	int mRecvUsed = 0;
	std::queue<PacketBuffer*> mRecvQueue;
	//std::queue<PacketBlock*> mSendQueue;
	RingBuffer mSendRingBuffer{ 65536 };
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
	void ConnectToServer(const char* ipAddress = "127.0.0.1", int port = 9000);
	void ReleaseServer();

	void Awake();
	void CheckConnect();

public: // Process

	void NetworkUpdate();
	void GameRecvUpdate();
	void GameSendUpdate();

	void ProcessPacket(PacketBuffer* packet);
	void PushSendData(const uint8_t* data, size_t size);

};

