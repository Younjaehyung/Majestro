#pragma once
#include "pch.h"
#pragma comment(lib, "ws2_32") // ws2_32.lib 링크


enum ServerPacketType : uint16_t
{
	S2C_LoginResult = 1000,
	S2C_EnterGameResult = 1001,
	C2S_Login = 2000,
	C2S_EnterGame = 2001,
};

struct PacketHeader
{
	uint16_t Size{};
	uint16_t Type{};
	uint16_t ClientId{};
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
	Network();
public:
	static Network& GetInstance() {
		static Network instance;
		return instance;
	}


	void Initialize();
	void ConnectToServer(const char* ipAddress = "127.0.0.1", int port = 9000);
	void ReleaseServer();
	void SendData(const char* data, int length);
	int ReceiveData(char* buffer, int length);

};

