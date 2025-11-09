#include "pch.h"
#include "Network.h"

Network::Network()
{
	mWsaData={};
	mSock={};
	mServerAddr={};
}

void Network::Initialize() {

	Network::GetInstance();

	if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0) {
		cout << "WSA creation failed with error: " << std::endl;
		return;
	}

	
	mSock = socket(AF_INET, SOCK_STREAM, 0);
	bool flag = true;
	setsockopt(mSock, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (mSock == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		return;
	}

}

void Network::ConnectToServer(const char* ipAddress, int port)
{
	mServerAddr.sin_family = AF_INET;
	
	inet_pton(AF_INET, ipAddress, &mServerAddr.sin_addr);

	mServerAddr.sin_port = htons(port);

	int r = connect(mSock, (sockaddr*)&mServerAddr, sizeof(mServerAddr));
	if(r == SOCKET_ERROR) {
		std::cout << "Failed to connect to server." << std::endl;
		delete this;
	}

	PacketHeader buffer{ sizeof(buffer),ServerPacketType::C2S_Login,0 };
	SendData((char*)&buffer, sizeof(buffer));
	ReceiveData((char*)&buffer, sizeof(buffer));

	mClientId = buffer.ClientId;

	std::cout << "Received packet of type: " << buffer.Type << " with ClientId: " << buffer.ClientId << std::endl;
}

void Network::ReleaseServer()
{
	closesocket(mSock);
	WSACleanup();
}

void Network::SendData(const char* data, int length)
{
	int result = 0;
	while (true)
	{
	result = send(mSock, data, length, 0);
		if(result == SOCKET_ERROR) {
			
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
			return;
		}
	}
	
}

int Network::ReceiveData(char* buffer, int length)
{
	while (true) {
		int data = recv(mSock, buffer, length, 0);
		if (data == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

		}
		return data;
	}

	
}
