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


	fd_set readSet;

	while (mIsRunning) {
		FD_ZERO(&readSet);
		FD_SET(mSock, &readSet);

		timeval timeOut{0,1000};
		int result = select(0, &readSet, nullptr, nullptr, &timeOut);

		if (result > 0) {
			std::cout << "Time Out: " << result << std::endl;
			// 데이터 수신 가능 상태
			if (FD_ISSET(mSock, &readSet)) {
				mRecvBuffer.Clean();

				int recvLen = recv(mSock, (char*)mRecvBuffer.WritePos(), mRecvBuffer.FreeSize(), 0);

				if (recvLen > 0) {
					if (!mRecvBuffer.OnWrite(recvLen))
					{
						std::cout << "RecvBuffer OnWrite Error" << std::endl;
						return;
					}
					while (true)
					{
						int32 ret = Onrecv(mRecvBuffer.ReadPos(), mRecvBuffer.DataSize());

						if (ret < 0 || ret > mRecvBuffer.DataSize())
						{
							std::cout << "OnRecv Error" << std::endl;
							return;;
						}
						else if (ret == 0)
						{
							break;
						}
						mRecvBuffer.OnRead(ret);
						
					}
				}
				else if (result == SOCKET_ERROR) {
					int32 errorCode = WSAGetLastError();
					if (errorCode != WSAEWOULDBLOCK)
					{
						continue;
					}
					LogDebug("select failed");
					ReleaseServer();
					std::cout << "select failed" << std::endl;
					return;
				}
				else {
					// 연결 종료
					ReleaseServer();
					std::cout << "Connection closed by server." << std::endl;
					return;
				}

			}
		}
		
	}
	

}

void Network::GameRecvUpdate()
{
		//RecvBuffer localQueue;
		//{
		//	std::lock_guard<std::mutex> lock(mQueueMutex);
		//	if (mRecvBuffer.empty()) return;
		//	std::swap(mRecvBuffer, localQueue);
		//}


		//while (!localQueue.empty()) {
		//	PacketBlock* packet = localQueue.front();
		//	localQueue.pop();

		//	ProcessPacket(packet);

		//	PacketPool::Release(packet);
		//}
	
}

void Network::GameSendUpdate()
{
	int result = 0;

	uint8_t* ptr1 = nullptr;
	uint8_t* ptr2 = nullptr;
	size_t size1 = 0, size2 = 0;

	// 1. 현재 보낼 수 있는 데이터 위치 확보
	while(!mSendBuffer.empty())
	{
		SendBuffer* sb = mSendBuffer.front();
		size_t remain = sb->Capacity - sb->ReadPos;
		int len = send(mSock, (char*)(sb->Data + sb->ReadPos),
			remain, 0);
		if (len < 0){
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				// 지금은 더 못 보냄  다음 select 때 재시도
				break;
			}
		}
		else if (len == 0)
		{
			// 연결 종료
			ReleaseServer();
			return;
		}
		else
		{
			sb->ReadPos += len;
			if (sb->ReadPos >= sb->Capacity)
			{
				// 다 보냈으면 큐에서 제거
				mSendBuffer.pop();
				PacketPool::Release(sb);
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

int32 Network::Onrecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header;
		::memcpy(&header, buffer + processLen, sizeof(PacketHeader));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다

		if (header.Size < sizeof(PacketHeader))
			return -1; // 프로토콜 오류

		if (dataSize < header.Size)
			break; // 아직 덜 옴

		// 패킷 조립 성공
		ProcessPacket(buffer + processLen, header.Size);

		processLen += header.Size;
	}

	return processLen;
}


void Network::ProcessPacket(BYTE* buffer, int32 len) {

	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	BYTE* payload = buffer + sizeof(PacketHeader);
	int32 payloadSize = header.Size - sizeof(PacketHeader);


	switch (header.PacketType) {
		case KPOSITION: {
			auto data = reinterpret_cast<MovePacketData*>(&payload);




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
			auto data = reinterpret_cast<SyncPacketData*>(&payload);

			std::cout << "Received Sync Packet: ClientID=" << data->clientId
				<< ", RhythmTime=" << data->rhythmTime << std::endl;

			// 리듬 동기화 및 서버 시간 보정
			//m_timeSystem->SyncWithServer(data->serverTime);
			break;
		}
	}
}

void Network::PushSendData(const uint8_t* data, size_t size)
{
	SendBuffer* packet = PacketPool::Acquire();
	packet->SetData(data, static_cast<uint16_t>(size));


	mSendBuffer.push(packet);

}
