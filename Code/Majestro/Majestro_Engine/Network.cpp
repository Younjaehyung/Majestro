#include "pch.h"
#include "Network.h"
#include "PacketHelper.h"

SpscRingQueue<SendRequest, 1024>		gSendBuffer;
SpscRingQueue<InputCommand, 1024>	gRecvBuffer;

Network::Network()
{
	mWsaData={};
	mTcpSocket = INVALID_SOCKET;
	mUdpSocket = INVALID_SOCKET;
	mServerTcpAddr={};
	mServerUdpAddr={};
	mIsRunning = false;

}

Network::~Network()
{
	mIsRunning = false;
	if (mNetworkThread.joinable()) {
		mNetworkThread.join();
	}
	ReleaseServer();
	WSACleanup();
}

void Network::Initialize() {

	SendBufferManager::Initialize(256);


	if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0) {
		cout << "WSA creation failed with error: " << std::endl;
		return;
	}
}

bool Network::Awake()
{

	if(ConnectToServer())
	{
		std::cout << "Connected to server successfully." << std::endl;
		return true;
	}
	else
	{
		std::cout << "Failed to awake to server." << std::endl;
		return false;
	}
}

bool Network::ConnectToServer(const char* ipAddress, int port)
{
	if (mIsRunning) return true;
	mTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	mUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	mServerTcpAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ipAddress, &mServerTcpAddr.sin_addr);
	mServerTcpAddr.sin_port = htons(port);

	mServerUdpAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ipAddress, &mServerUdpAddr.sin_addr);
	mServerUdpAddr.sin_port = htons(port + 1);


	int r = connect(mTcpSocket, (sockaddr*)&mServerTcpAddr, sizeof(mServerTcpAddr));
	if(r == SOCKET_ERROR) {
		std::cout << "Failed to connect to server." << std::endl;
		ReleaseServer();
		return false;
	}

	sockaddr_in localUdp{};
	localUdp.sin_family = AF_INET;
	localUdp.sin_addr.s_addr = htonl(INADDR_ANY);
	localUdp.sin_port = htons(0); // 0 = OS가 사용 가능한 포트를 자동 할당

	if (::bind(mUdpSocket, (sockaddr*)&localUdp, sizeof(localUdp)) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		std::cout << "UDP bind failed: " << err << std::endl;
		ReleaseServer();
		return false;
	}



	u_long one = 1;
	ioctlsocket(mTcpSocket, FIONBIO, &one);

	u_long one1 = 1;
	ioctlsocket(mUdpSocket, FIONBIO, &one1);


	if (mTcpSocket == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		ReleaseServer();
		return false;
	}
	if (mUdpSocket == INVALID_SOCKET) {
		int32_t error = WSAGetLastError();
		cout << "Socket creation failed with error: " << error << std::endl;
		ReleaseServer();
		return false;
	}
	


	/*bool flag = true; // nagle
setsockopt(mSock, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(flag));*/


	mIsRunning = true;
	mNetworkThread = std::thread(&Network::NetworkUpdate, this);

	return true;
}

void Network::ReleaseServer()
{
	SendBufferManager::Shutdown();

	if (mTcpSocket != INVALID_SOCKET)
		closesocket(mTcpSocket);

	if (mUdpSocket != INVALID_SOCKET)
		closesocket(mUdpSocket);


	mTcpSocket = INVALID_SOCKET;
	mUdpSocket = INVALID_SOCKET;

	
}

void Network::NetworkUpdate()
{

	while (mIsRunning) {
		OnRecvPacket();
		OnSendPacket();
	}
	ReleaseServer();
}

void Network::Shutdown()
{
	std::cout << "Network Shutdown called." << std::endl;
	mIsRunning = false;
}

void Network::Stop()
{
	std::cout << "Network Shutdown called." << std::endl;
	mIsRunning = false;
	if (mNetworkThread.joinable()) {
		mNetworkThread.join();
	}
}





///////////////////////////////////
//SEND
///////////////////////////////////

void Network::OnSendPacket()
{
	if (!mIsRunning) return;

	PrepareSendData();	//send

	while (!mSendBuffer.empty()) {

		
		SendBuffer* sendBuffer = mSendBuffer.front();

		switch (sendBuffer->Protocol)
		{
		case NetProtocol::TCP:
		{

			uint32 remain = sendBuffer->Capacity - sendBuffer->ReadPos;

			int len = send(mTcpSocket, (char*)sendBuffer->Data + sendBuffer->ReadPos, remain, 0);
			//ProcessSendData();
			if (len == 0)
			{
				// 연결이 종료됨
				Shutdown();
				return;
			}
			else if (len < 0)
			{
				int err = WSAGetLastError();
				if (err == WSAEWOULDBLOCK)
				{

					break;
				}

				// 치명적 오류
				Shutdown();
				return;
			}
			// 부분/전체 전송 반영
			sendBuffer->ReadPos += len;

			if (sendBuffer->ReadPos > sendBuffer->Capacity)
			{
				Shutdown();
				SendBufferManager::Release(sendBuffer);
				return;
			}
			if (sendBuffer->Capacity > sendBuffer->ReadPos)
			{
				continue; // 아직 덜 보냄
			}

			mSendBuffer.pop();
			SendBufferManager::Release(sendBuffer);

			break;
		}
		case NetProtocol::UDP:
		{


			int len = sendto(mUdpSocket, (char*)sendBuffer->Data, sendBuffer->Capacity, 0,
				(sockaddr*)&mServerUdpAddr, sizeof(mServerUdpAddr));

			if (len > 0)
			{
				// 전송 성공
				mSendBuffer.pop();
				SendBufferManager::Release(sendBuffer);
			}



			break;
		}
		default:
		{	// 알 수 없는 프로토콜
			mSendBuffer.pop();
			SendBufferManager::Release(sendBuffer);
			break;
		}
		}


	}

}


void Network::PrepareSendData()
{
	while (gSendBuffer.Pop(mSendData))
	{
		SendBuffer* sendBuffer = SendBufferManager::Acquire();
		SendRequestPacket::SerializePacket(mSendData, sendBuffer);
		mSendBuffer.push(sendBuffer);
	}

}


///////////////////////////////////
//RECV
///////////////////////////////////


void Network::OnRecvPacket()
{
	if (!mIsRunning) return;
	OnTCPNetworkUpdate();	// recv
	OnUDPNetworkUpdate();	// recv

}


void Network::OnTCPNetworkUpdate()
{
	if (!mIsRunning) return;
	// 수신 처리
	mTRecvBuffer.Clean();

	if (mTRecvBuffer.FreeSize() <= 0) {
		Shutdown();
		return;
	}

	int len = recv(mTcpSocket, (char*)mTRecvBuffer.WritePos(),
		mTRecvBuffer.FreeSize(), 0);

	if (len > 0) {
		if (!mTRecvBuffer.OnWrite(len)) {
			Shutdown();
			return;
		}
		while (true) {
			int32 ret = OnTcpRecv(mTRecvBuffer.ReadPos(), mTRecvBuffer.DataSize());
			if (ret < 0 || ret > mTRecvBuffer.DataSize()) {
				Shutdown();
				return;
			}
			else if (ret == 0) {
				break;
			}
			mTRecvBuffer.OnRead(ret);
		}
	}
	else if (len == 0) {
		Shutdown();
		return;
	}
	else {
		int32 errorCode = WSAGetLastError();
		if (errorCode != WSAEWOULDBLOCK) {
			Shutdown();
			return;
		}
	}
	
}

int32 Network::OnTcpRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;


	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketTcpHeader))
			return 0;

		PacketTcpHeader header;
		::memcpy(&header, buffer, sizeof(PacketTcpHeader));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다

		if (header.Header.Size < sizeof(PacketTcpHeader))
			return -1; // 프로토콜 오류

		if (dataSize < header.Header.Size)
			return 0;

		// 패킷 조립 성공

		ProcessPacket::ProcessPackets(mInputCommand, buffer);
		if (mInputCommand.Type == S2C_PKT_LOGIN) {

			const S2C_LoginPacket* loginResp =
				(mInputCommand.ViewAs<S2C_LoginPacket>());
			
			mClientId = loginResp->clientId;

			C2S_LoginPacket loginPkt;
			loginPkt.clientId = mClientId;
			loginPkt.Sequence = 0;
			loginPkt.SessionId = mClientId;

			int len = sendto(mUdpSocket, (char*)&loginPkt, sizeof(C2S_LoginPacket), 0,
				(sockaddr*)&mServerUdpAddr, sizeof(sockaddr_in));
		}

		gRecvBuffer.Push(mInputCommand);


		processLen += header.Header.Size;
	}

	return processLen;
}


void Network::OnUDPNetworkUpdate()
{
	if (!mIsRunning) return;
	// UDP 수신 처리
	sockaddr_in fromAddr{};
	int fromLen = sizeof(fromAddr);
	
	int len = recvfrom(mUdpSocket, (char*)mURecvBuffer, BUFSIZE, 0,
		(sockaddr*)&fromAddr, &fromLen);
	
	if (len > 0) {
		// UDP 패킷 처리
		ProcessPacket::ProcessPackets(mInputCommand, mURecvBuffer);
		gRecvBuffer.Push(mInputCommand);
	}
	else if (len == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		// FIX: 논블로킹에서 "지금 받을 게 없음"은 정상 상황
		if (err == WSAEWOULDBLOCK)
			return;

		// FIX: UDP에서 흔한 케이스(상대 포트 unreachable 등). 필요 시 무시 가능
		if (err == WSAECONNRESET)
			return;

		// 그 외는 진짜 오류로 보고 종료
		std::cout << "UDP recvfrom failed: " << err << std::endl;
		Shutdown();
		return;
	}

}
