#include "pch.h"
#include "Network.h"

Network::Network()
{
	mWsaData={};
	mSock={};
	mServerAddr={};
	mIsRunning = false;

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

void Network::ConnectToServer(const char* ipAddress, int port)
{

	mSock = socket(AF_INET, SOCK_STREAM, 0);


	mServerAddr.sin_family = AF_INET;
	
	inet_pton(AF_INET, ipAddress, &mServerAddr.sin_addr);

	mServerAddr.sin_port = htons(port);

	int r = connect(mSock, (sockaddr*)&mServerAddr, sizeof(mServerAddr));
	if(r == SOCKET_ERROR) {
		std::cout << "Failed to connect to server." << std::endl;
		delete this;
	}

	bool flag = true;
	setsockopt(mSock, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (mSock == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		return;
	}

	mIsRunning = true;
	mNetworkThread = std::thread(&Network::NetworkUpdate, this);

}


void Network::NetworkUpdate()
{
	timeval timeOut;
	timeOut.tv_sec = 0;
	timeOut.tv_usec = 1000; // 1ms

	fd_set readSet;

	while (mIsRunning) {
		FD_ZERO(&readSet);
		FD_SET(mSock, &readSet);

		int result = select(0, &readSet, nullptr, nullptr, &timeOut);

		if (result > 0) {
			// 데이터 수신 가능 상태
			if (FD_ISSET(mSock, &readSet)) {
				PacketBlock* packet = PacketPool::Acquire();

				int serverAddrLen = sizeof(mServerAddr);
				int recvLen = recvfrom(mSock, (char*)packet->Data, MAX_PACKET_SIZE, 0,
					(sockaddr*)&mServerAddr, &serverAddrLen);

				if (recvLen > 0) {
					packet->Header.Size = (uint16_t)recvLen;
					// TO - DO Lock-freeQueue로 바꿀 것.
					std::lock_guard<std::mutex> lock(mQueueMutex);
					mRecvQueue.push(packet);
				}
				else {
					PacketPool::Release(packet);
				}
			}
		}
		else if (result == SOCKET_ERROR) {
			err_display("select failed");
			return;
		}
		// result == 0 인 경우는 타임아웃이므로 루프 다시 실행
	}
	

}

void Network::GameRecvUpdate()
{
		std::queue<PacketBlock*> localQueue;
		{
			std::lock_guard<std::mutex> lock(mQueueMutex);
			if (mRecvQueue.empty()) return;
			std::swap(mRecvQueue, localQueue);
		}


		while (!localQueue.empty()) {
			PacketBlock* packet = localQueue.front();
			localQueue.pop();

			ProcessPacket(packet);

			PacketPool::Release(packet);
		}
	
}

void Network::GameSendUpdate()
{
}

void Network::ReleaseServer()
{
	closesocket(mSock);
	WSACleanup();
}

void Network::CheckConnect()
{
	// 서버와 연결이 끊겼는지 확인
	

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


	while (mRecvUsed >= sizeof(PacketHeader)) {
		PacketHeader* header = (PacketHeader*)mRecvBuf;
		uint16 packetSize = header->Size;

		if (mRecvUsed < packetSize)
			break;
		//ProcessPacket((BYTE*)recvBuf, packetSize);
		memmove(mRecvBuf, mRecvBuf + packetSize, mRecvUsed - packetSize);
		mRecvUsed -= packetSize;
	}

}

void Network::ProcessPacket(PacketBlock* packet) {
	switch (packet->Header.PacketType) {
		case KPOSITION: {
			auto data = reinterpret_cast<MovePacketData*>(packet->Data);

			// 게임 월드의 해당 캐릭터 위치 갱신
			// 여기서 직접 위치를 세팅하면 '순간이동'처럼 보이므로, 
			// 목표 위치(TargetPos)만 설정하고 World::Tick에서 보간(Lerp)
			//Player* player = m_world->FindPlayer(data->playerId);
			//if (player) {
			//	player->SetTargetPosition(data->x, data->y, data->z);
			//}
			break;
		}
		case KSYNC: {
			auto data = reinterpret_cast<SyncPacketData*>(packet->Data);
			// 리듬 동기화 및 서버 시간 보정
			//m_timeSystem->SyncWithServer(data->serverTime);
			break;
		}
	}
}