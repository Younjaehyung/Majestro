#include "pch.h"
#include "Network.h"

Network::Network()
{
	mWsaData={};
	mSock={};
	mServerAddr={};
}

void Network::Initialize() {


	if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0) {
		cout << "WSA creation failed with error: " << std::endl;
		return;
	}

	


}

void Network::Awake()
{

	ConnectToServer();

}

void Network::Update()
{
	CheckConnect();
	ReceiveData();
	SendData();
}



void Network::ConnectToServer(const char* ipAddress, int port)
{

	mSock = socket(AF_INET, SOCK_STREAM, 0);
	bool flag = true;
	setsockopt(mSock, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (mSock == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		return;
	}

	mServerAddr.sin_family = AF_INET;
	
	inet_pton(AF_INET, ipAddress, &mServerAddr.sin_addr);

	mServerAddr.sin_port = htons(port);

	int r = connect(mSock, (sockaddr*)&mServerAddr, sizeof(mServerAddr));
	if(r == SOCKET_ERROR) {
		std::cout << "Failed to connect to server." << std::endl;
		delete this;
	}

	//PacketHeader buffer{ sizeof(buffer),ServerPacketType::C2S_Login,0 };
	//SendData((char*)&buffer, sizeof(buffer));
	//ReceiveData((char*)&buffer, sizeof(buffer));

	//mClientId = buffer.ClientId;

	//std::cout << "Received packet of type: " << buffer.Type << " with ClientId: " << buffer.ClientId << std::endl;
}

void Network::ReleaseServer()
{
	closesocket(mSock);
	WSACleanup();
}

void Network::CheckConnect()
{
}

void Network::SendData()
{
	int result = 0;
	while (true)
	{
	//result = send(mSock, data, length, 0);
		if(result == SOCKET_ERROR) {
			
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
			return;
		}
	}
	
}

int Network::ReceiveData()
{

	int ret = recv(mSock, mRecvBuf + mRecvUsed, sizeof(mRecvBuf) - mRecvUsed, 0);
	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)// 받을 데이터가 없음
			return -1;  // 즉시 리턴
		else
		{
			// 실제 에러
			std::cout << "[Client] recv error " << err << std::endl;
			return -1;
		}
	}
	if (ret == 0)
	{
		// 서버 연결 종료
		std::cout << "[Client] server closed connection.\n";
		return -1;
	}

	mRecvUsed += ret;
	//std::cout << "recv data size : " << ret << " , total data size: " << recvUsed << std::endl;


	while (mRecvUsed >= sizeof(PacketDataInfo)) {
		PacketDataInfo* header = (PacketDataInfo*)mRecvBuf;
		uint16 packetSize = header->Size;

		if (mRecvUsed < packetSize)
			break;
		//ProcessPacket((BYTE*)recvBuf, packetSize);
		memmove(mRecvBuf, mRecvBuf + packetSize, mRecvUsed - packetSize);
		mRecvUsed -= packetSize;
	}

}

