#include "pch.h"
#include "VirtualClient.h"
#include <random>

static std::mt19937 sRng(std::random_device{}());
static std::uniform_real_distribution<float> sDist(-1.0f, 1.0f);
static std::uniform_int_distribution<int> sPlayerTypeDist(0, 2);

const char* ClientStateToString(ClientState state)
{
	switch (state) {
	case ClientState::Disconnected:  return "Disconnected";
	case ClientState::Connecting:    return "Connecting";
	case ClientState::TcpConnected:  return "TcpConnected";
	case ClientState::LoggedIn:      return "LoggedIn";
	case ClientState::SceneChanging: return "SceneChanging";
	case ClientState::GameStarting:  return "GameStarting";
	case ClientState::Playing:       return "Playing";
	case ClientState::Failed:        return "Failed";
	default:                         return "Unknown";
	}
}

VirtualClient::VirtualClient(uint32 id)
	: mId(id), mTcpRecvBuffer(1024)
{
}

VirtualClient::~VirtualClient()
{
	Disconnect();
}

bool VirtualClient::StartConnect(const char* ip, int tcpPort, int udpPort)
{
	// Create TCP socket
	mTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mTcpSocket == INVALID_SOCKET) return false;

	// Create UDP socket
	mUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mUdpSocket == INVALID_SOCKET) {
		closesocket(mTcpSocket);
		mTcpSocket = INVALID_SOCKET;
		return false;
	}

	// Set server addresses
	mServerTcpAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &mServerTcpAddr.sin_addr);
	mServerTcpAddr.sin_port = htons(tcpPort);

	mServerUdpAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &mServerUdpAddr.sin_addr);
	mServerUdpAddr.sin_port = htons(udpPort);

	// Bind UDP to any local port (OS auto-assign)
	sockaddr_in localUdp{};
	localUdp.sin_family = AF_INET;
	localUdp.sin_addr.s_addr = htonl(INADDR_ANY);
	localUdp.sin_port = htons(0);
	if (::bind(mUdpSocket, (sockaddr*)&localUdp, sizeof(localUdp)) == SOCKET_ERROR) {
		closesocket(mTcpSocket);
		closesocket(mUdpSocket);
		mTcpSocket = INVALID_SOCKET;
		mUdpSocket = INVALID_SOCKET;
		return false;
	}

	// Set non-blocking
	u_long one = 1;
	ioctlsocket(mTcpSocket, FIONBIO, &one);
	ioctlsocket(mUdpSocket, FIONBIO, &one);

	// Non-blocking connect
	int r = connect(mTcpSocket, (sockaddr*)&mServerTcpAddr, sizeof(mServerTcpAddr));
	if (r == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			closesocket(mTcpSocket);
			closesocket(mUdpSocket);
			mTcpSocket = INVALID_SOCKET;
			mUdpSocket = INVALID_SOCKET;
			return false;
		}
	}

	SetState(ClientState::Connecting);
	return true;
}

void VirtualClient::OnConnectComplete()
{
	// Check if connect succeeded
	int err = 0;
	int errLen = sizeof(err);
	getsockopt(mTcpSocket, SOL_SOCKET, SO_ERROR, (char*)&err, &errLen);
	if (err != 0) {
		SetState(ClientState::Failed);
		return;
	}
	SetState(ClientState::TcpConnected);
}

void VirtualClient::Disconnect()
{
	if (mTcpSocket != INVALID_SOCKET) {
		closesocket(mTcpSocket);
		mTcpSocket = INVALID_SOCKET;
	}
	if (mUdpSocket != INVALID_SOCKET) {
		closesocket(mUdpSocket);
		mUdpSocket = INVALID_SOCKET;
	}
	// Clean send queues
	while (!mTcpSendQueue.empty()) {
		SendBufferManager::Release(mTcpSendQueue.front());
		mTcpSendQueue.pop();
	}
	while (!mUdpSendQueue.empty()) {
		SendBufferManager::Release(mUdpSendQueue.front());
		mUdpSendQueue.pop();
	}
	if (mState != ClientState::Failed)
		mState = ClientState::Disconnected;
}

void VirtualClient::SetState(ClientState newState)
{
	mState = newState;
	mStateChangeTime = Clock::now();
}

void VirtualClient::Update(float dt)
{
	auto now = Clock::now();
	float elapsed = std::chrono::duration<float>(now - mStateChangeTime).count();

	switch (mState) {
	case ClientState::TcpConnected:
		// Waiting for S2C_LoginPacket - timeout 10s
		if (elapsed > 10.0f) {
			SetState(ClientState::Failed);
		}
		break;

	case ClientState::LoggedIn:
	{
		// Send C2S_LoginPacket via UDP to register
		C2S_LoginPacket loginPkt(mClientId);
		SendUdp(&loginPkt, sizeof(loginPkt));

		// Send scene change request
		C2S_SceneChangePacket sceneReq(SceneId::FirstGame);
		SendTcp(&sceneReq, sizeof(sceneReq));
		SetState(ClientState::SceneChanging);
		break;
	}

	case ClientState::SceneChanging:
		if (elapsed > 10.0f) {
			SetState(ClientState::Failed);
		}
		break;

	case ClientState::GameStarting:
		if (elapsed > 10.0f) {
			SetState(ClientState::Failed);
		}
		break;

	case ClientState::Playing:
		SimulateGameplay(dt);
		break;

	default:
		break;
	}
}

void VirtualClient::OnTcpReadable()
{
	mTcpRecvBuffer.Clean();
	if (mTcpRecvBuffer.FreeSize() <= 0) {
		SetState(ClientState::Failed);
		return;
	}

	int len = recv(mTcpSocket, (char*)mTcpRecvBuffer.WritePos(), mTcpRecvBuffer.FreeSize(), 0);
	if (len > 0) {
		mBytesRecv += len;
		if (!mTcpRecvBuffer.OnWrite(len)) {
			SetState(ClientState::Failed);
			return;
		}
		// Parse all complete packets
		while (true) {
			int32 dataSize = mTcpRecvBuffer.DataSize();
			if (dataSize < (int32)sizeof(PacketTcpHeader)) break;

			PacketTcpHeader header;
			::memcpy(&header, mTcpRecvBuffer.ReadPos(), sizeof(PacketTcpHeader));

			if (header.Header.Size < sizeof(PacketTcpHeader)) {
				SetState(ClientState::Failed);
				return;
			}
			if (dataSize < (int32)header.Header.Size) break;

			ProcessTcpPacket(mTcpRecvBuffer.ReadPos(), header.Header.Size);
			mTcpRecvBuffer.OnRead(header.Header.Size);
			mPacketsRecv++;
		}
	}
	else if (len == 0) {
		SetState(ClientState::Failed);
	}
	else {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			SetState(ClientState::Failed);
		}
	}
}

void VirtualClient::OnUdpReadable()
{
	sockaddr_in fromAddr{};
	int fromLen = sizeof(fromAddr);
	int len = recvfrom(mUdpSocket, (char*)mUdpRecvBuffer, sizeof(mUdpRecvBuffer), 0,
		(sockaddr*)&fromAddr, &fromLen);
	if (len > 0) {
		mBytesRecv += len;
		mPacketsRecv++;
		ProcessUdpPacket(mUdpRecvBuffer, len);
	}
}

void VirtualClient::ProcessTcpPacket(BYTE* buffer, uint32 size)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	switch (header.PacketType) {
	case PKT_Type::PKT_LOGIN:
	{
		LoginPacket pkt;
		::memcpy(&pkt, buffer, sizeof(LoginPacket));
		mClientId = pkt.clientId;
		if (mState == ClientState::TcpConnected) {
			SetState(ClientState::LoggedIn);
		}
		break;
	}
	case PKT_Type::S2C_PKT_LOGIN:
	{
		S2C_LoginPacket pkt;
		::memcpy(&pkt, buffer, sizeof(S2C_LoginPacket));
		mClientId = pkt.clientId;
		if (mState == ClientState::TcpConnected) {
			SetState(ClientState::LoggedIn);
		}
		break;
	}
	case PKT_Type::S2C_SCENE_CHANGE_RESULT:
	{
		if (mState == ClientState::SceneChanging) {
			// Send game start request
			C2S_StartGamePacket startPkt(mClientId, (uint8)sPlayerTypeDist(sRng));
			SendUdp(&startPkt, sizeof(startPkt));
			SetState(ClientState::GameStarting);
		}
		break;
	}
	case PKT_Type::S2C_GAME_START:
	{
		if (mState == ClientState::GameStarting) {
			SetState(ClientState::Playing);
		}
		break;
	}
	// Other S2C packets - just consume
	case PKT_Type::S2C_PKT_SPAWN:
	case PKT_Type::S2C_PKT_SPAWNS:
	case PKT_Type::S2C_PKT_STATE:
	case PKT_Type::S2C_PKT_HEALTH:
	case PKT_Type::S2C_PKT_ARMOR:
	case PKT_Type::S2C_PKT_AMMO:
	case PKT_Type::S2C_PKT_HIT_CONFIRM:
	case PKT_Type::S2C_PKT_COLLISION:
	case PKT_Type::S2C_PKT_BULLET_ACTIVATE:
	case PKT_Type::S2C_PKT_BULLET_DEACTIVATE:
	case PKT_Type::S2C_PKT_EFFECT_SPAWN:
	case PKT_Type::S2C_PKT_RESPAWN:
	case PKT_Type::S2C_PKT_SYNC:
	case PKT_Type::S2C_PKT_POS:
		break;
	default:
		break;
	}
}

void VirtualClient::ProcessUdpPacket(BYTE* buffer, uint32 size)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	switch (header.PacketType) {
	case PKT_Type::S2C_PKT_MOVE:
		// Position updates - ignored in stress test
		break;
	case PKT_Type::S2C_GAME_START:
		if (mState == ClientState::GameStarting) {
			SetState(ClientState::Playing);
		}
		break;
	default:
		break;
	}
}

void VirtualClient::SendTcp(const void* data, uint32 size)
{
	SendBuffer* buf = SendBufferManager::Acquire();
	buf->SetData(data, size, NetProtocol::TCP);
	mTcpSendQueue.push(buf);
	mPacketsSent++;
	mBytesSent += size;
}

void VirtualClient::SendUdp(const void* data, uint32 size)
{
	SendBuffer* buf = SendBufferManager::Acquire();
	buf->SetData(data, size, NetProtocol::UDP);
	mUdpSendQueue.push(buf);
	mPacketsSent++;
	mBytesSent += size;
}

void VirtualClient::OnTcpWritable()
{
	while (!mTcpSendQueue.empty()) {
		SendBuffer* buf = mTcpSendQueue.front();
		uint32 remain = buf->Capacity - buf->ReadPos;
		int sent = send(mTcpSocket, (char*)buf->Data + buf->ReadPos, remain, 0);
		if (sent > 0) {
			buf->ReadPos += sent;
			if (buf->ReadPos >= buf->Capacity) {
				mTcpSendQueue.pop();
				SendBufferManager::Release(buf);
			}
		}
		else if (sent == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) break;
			SetState(ClientState::Failed);
			return;
		}
		else {
			SetState(ClientState::Failed);
			return;
		}
	}
}

void VirtualClient::OnUdpWritable()
{
	while (!mUdpSendQueue.empty()) {
		SendBuffer* buf = mUdpSendQueue.front();
		int sent = sendto(mUdpSocket, (char*)buf->Data, buf->Capacity, 0,
			(sockaddr*)&mServerUdpAddr, sizeof(mServerUdpAddr));
		if (sent > 0) {
			mUdpSendQueue.pop();
			SendBufferManager::Release(buf);
		}
		else {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) break;
			// UDP errors are usually non-fatal - release and continue
			mUdpSendQueue.pop();
			SendBufferManager::Release(buf);
			break;
		}
	}
}

void VirtualClient::SimulateGameplay(float dt)
{
	// Change direction periodically
	mDirectionTimer -= dt;
	if (mDirectionTimer <= 0) {
		mMoveX = sDist(sRng);
		mMoveZ = sDist(sRng);
		mYaw += sDist(sRng) * 90.0f;
		mDirectionTimer = 1.0f + sDist(sRng) * 0.5f; // 0.5~1.5s
	}

	// Update position
	float speed = 5.0f;
	mPosX += mMoveX * speed * dt;
	mPosZ += mMoveZ * speed * dt;

	// Send move packet
	mMoveTimer -= dt;
	if (mMoveTimer <= 0) {
		C2S_MovePacket movePkt;
		movePkt.netEntityId = mClientId;
		movePkt.Seq = ++mMoveSeq;
		movePkt.Dt = dt;
		movePkt.MoveX = mMoveX;
		movePkt.MoveY = 0;
		movePkt.MoveZ = mMoveZ;
		movePkt.Yaw = mYaw;
		movePkt.Pitch = 0;
		movePkt.Buttons = 0;
		movePkt.SessionId = mClientId;
		SendUdp(&movePkt, sizeof(movePkt));
		mMoveTimer = mMoveSendInterval;
	}

	// Send action packet periodically
	mActionTimer -= dt;
	if (mActionTimer <= 0) {
		C2S_ActionPacket actionPkt;
		actionPkt.netEntityId = mClientId;
		actionPkt.Buttons = 1;
		actionPkt.Yaw = mYaw;
		actionPkt.Pitch = 0;
		SendTcp(&actionPkt, sizeof(actionPkt));
		mActionTimer = mActionInterval;
	}
}
