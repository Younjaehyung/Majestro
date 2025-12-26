#pragma once
#include "pch.h"
#include <queue>
#include "Packet.h"

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

	const char* SERVERIP = "127.0.0.1";
	char mRecvBuf[BUFSIZ] = {};
	int mRecvUsed = 0;
	std::queue<PendingSend> sendQ;


	Network();
public: // Init
	static Network& GetInstance() {
		static Network instance;
		return instance;
	}


	void Initialize();
	void ConnectToServer(const char* ipAddress = "127.0.0.1", int port = 9000);
	void ReleaseServer();


public: // Process

	void Update();
	void Awake();
	void CheckConnect();

	void ProcessPacket(const char* data, int length);

	void SendData();
	int ReceiveData();


};

