#include "pch.h"
#include "Network.h"

Network::Network()
{
	mWsaData={};
	mSock={};
	mServerAddr={};
	mIsRunning = false;

}

Network::~Network()
{
	mIsRunning = false;
	if (mNetworkThread.joinable()) {
		mNetworkThread.join();
	}
	ReleaseServer();
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
		ReleaseServer();
		return;
	}

	u_long one = 1;
	ioctlsocket(mSock, FIONBIO, &one);

	bool flag = true;
	setsockopt(mSock, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (mSock == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		ReleaseServer();
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
				int recvLen = recv(mSock, (char*)packet->Data, MAX_PACKET_SIZE, 0);

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
			ReleaseServer();
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
	int result = 0;

	uint8_t* ptr1 = nullptr;
	uint8_t* ptr2 = nullptr;
	size_t size1 = 0, size2 = 0;

	// 1. 현재 보낼 수 있는 데이터 위치 확보
	mSendRingBuffer.Peek(&ptr1, size1, &ptr2, size2);

	if (size1 > 0) {
		// 첫 번째 영역 송신 시도
		int sent = send(mSock, (const char*)ptr1, (int)size1, 0);
		if (sent > 0) {
			mSendRingBuffer.Consume(sent);

			// 첫 번째 영역을 다 보냈고, 두 번째 영역(Wrap-around)이 있다면 연속 송신 시도
			if (sent == size1 && size2 > 0) {
				int sent2 = send(mSock, (const char*)ptr2, (int)size2, 0);
				if (sent2 > 0) mSendRingBuffer.Consume(sent2);
			}
		}
		else if (sent == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
				return;
			}
		}
	}

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

void Network::PushSendData(const uint8_t* data, size_t size)
{
	mSendRingBuffer.Push(data, size);
}
