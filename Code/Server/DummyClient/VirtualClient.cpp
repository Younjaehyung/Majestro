#include "pch.h"
#include "VirtualClient.h"
#include "StressTest.h"
#include <random>

static std::mt19937 sRng(std::random_device{}());
static std::uniform_real_distribution<float> sDist(-1.0f, 1.0f);

// 단조 증가 초 단위 타임스탬프(모든 클라 공통 기준). Sync RTT 측정용.
static double NowSeconds()
{
	static const auto epoch = std::chrono::steady_clock::now();
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - epoch).count();
}

const char* ClientStateToString(ClientState state)
{
	switch (state) {
	case ClientState::Disconnected:  return "Disconnected";
	case ClientState::Connecting:    return "Connecting";
	case ClientState::TcpConnected:  return "TcpConnected";
	case ClientState::Registering:   return "Registering";
	case ClientState::InLobby:       return "InLobby";
	case ClientState::CreatingRoom:  return "CreatingRoom";
	case ClientState::WaitingForRoom:return "WaitingForRoom";
	case ClientState::JoiningRoom:   return "JoiningRoom";
	case ClientState::InRoom:        return "InRoom";
	case ClientState::Ready:         return "Ready";
	case ClientState::SceneChanging: return "SceneChanging";
	case ClientState::GameStarting:  return "GameStarting";
	case ClientState::Playing:       return "Playing";
	case ClientState::Leaving:       return "Leaving";
	case ClientState::Failed:        return "Failed";
	default:                         return "Unknown";
	}
}

VirtualClient::VirtualClient(uint32 id)
	: mId(id), mTcpRecvBuffer(2048)
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

// ─── 상태 머신 ────────────────────────────────────────────────
void VirtualClient::Update(float dt)
{
	auto now = Clock::now();
	float elapsed = std::chrono::duration<float>(now - mStateChangeTime).count();

	switch (mState) {
	case ClientState::TcpConnected:
		// Waiting for S2C_LoginPacket - timeout 10s
		if (elapsed > 10.0f) SetState(ClientState::Failed);
		break;

	case ClientState::Registering:
	{
		// UDP 로그인으로 세션 등록(서버 홀 진입 + UDP 주소 등록)
		C2S_LoginPacket loginPkt(mClientId);
		SendUdp(&loginPkt, sizeof(loginPkt));
		// 방 목록 요청(홀 진입 트리거 보강)
		C2S_RoomListPacket listPkt;
		SendTcp(&listPkt, sizeof(listPkt));
		OnRegistered();
		break;
	}

	case ClientState::InLobby:
		if (mScenario == TestScenario::FullGame)
			TryStartRoomFlow();
		else
			UpdateLobbyChurn(dt);
		break;

	case ClientState::WaitingForRoom:
	{
		// Host 가 roomId 를 발급할 때까지 폴링
		uint32 roomId = mManager ? mManager->GetGroupRoomId(mRoomGroup) : 0;
		if (roomId != 0)
		{
			mRoomId = roomId;
			C2S_RoomJoinPacket joinPkt;
			joinPkt.roomId = roomId;
			SendTcp(&joinPkt, sizeof(joinPkt));
			SetState(ClientState::JoiningRoom);
		}
		else if (elapsed > 20.0f)
		{
			SetState(ClientState::Failed);
		}
		break;
	}

	case ClientState::CreatingRoom:
	case ClientState::JoiningRoom:
		if (elapsed > 15.0f) SetState(ClientState::Failed);
		break;

	case ClientState::InRoom:
		// 방 입장 직후 Ready 송신 (1회)
		if (!mReadySent)
		{
			SendRoomReady();
			if (mScenario == TestScenario::LobbyChurn)
				mChurnTimer = mChurnInterval; // 방에 잠깐 머문 뒤 퇴장
			SetState(ClientState::Ready);
		}
		break;

	case ClientState::Ready:
		if (mScenario == TestScenario::FullGame)
		{
			// Host 만 전원 Ready 를 확인하고 게임 시작을 요청
			if (mIsRoomHost && mRoomAllReady)
				SendSceneChangeToGame();
			else if (elapsed > 60.0f)
				SetState(ClientState::Failed); // 모이지 않으면 포기
		}
		else // LobbyChurn: 잠깐 머문 뒤 방을 떠남
		{
			mChurnTimer -= dt;
			if (mChurnTimer <= 0.0f)
			{
				C2S_RoomLeavePacket leavePkt;
				leavePkt.roomId = mRoomId;
				SendTcp(&leavePkt, sizeof(leavePkt));
				mRoomId = 0;
				mReadySent = false;
				SetState(ClientState::Leaving);
			}
		}
		break;

	case ClientState::SceneChanging:
		if (elapsed > 15.0f) SetState(ClientState::Failed);
		break;

	case ClientState::GameStarting:
		// C2S_GAME_START(UDP)는 유실 가능 → 본인 스폰을 받을 때까지 주기적 재전송.
		// (서버 SpawnPlayer 에 세션 중복 스폰 가드가 있어 재전송은 안전)
		mRoomActionTimer -= dt;
		if (mRoomActionTimer <= 0.0f) {
			C2S_StartGamePacket startPkt(mClientId, mRoomPlayerType);
			startPkt.SessionId = mClientId;
			SendUdp(&startPkt, sizeof(startPkt));
			mRoomActionTimer = 1.0f;
		}
		if (elapsed > 20.0f) SetState(ClientState::Failed);
		break;

	case ClientState::Playing:
		SimulateGameplay(dt);
		break;

	case ClientState::Leaving:
		// 서버 처리 후 홀로 복귀 (다음 churn 사이클)
		if (elapsed > 0.3f)
		{
			++mChurnCycle;
			mChurnTimer = 0.0f;
			SetState(ClientState::InLobby);
		}
		break;

	default:
		break;
	}
}

void VirtualClient::OnRegistered()
{
	mReadySent = false;
	mRoomAllReady = false;
	SetState(ClientState::InLobby);
}

// ─── FullGame: 방 구성 ────────────────────────────────────────
void VirtualClient::TryStartRoomFlow()
{
	mRoomAllReady = false;
	if (mIsRoomHost)
	{
		// Host: 방 생성 요청
		C2S_RoomCreatePacket createPkt;
		SendTcp(&createPkt, sizeof(createPkt));
		SetState(ClientState::CreatingRoom);
	}
	else
	{
		// Member: Host 가 roomId 발급할 때까지 대기
		SetState(ClientState::WaitingForRoom);
	}
}

// ─── LobbyChurn: 로비/방 churn ────────────────────────────────
void VirtualClient::UpdateLobbyChurn(float dt)
{
	// InLobby 진입 시 방 목록을 한 번 요청한 뒤, 방을 만들어 churn
	mChurnTimer -= dt;
	if (mChurnTimer > 0.0f)
		return;

	// 방 목록 요청 (홀 부하)
	C2S_RoomListPacket listPkt;
	SendTcp(&listPkt, sizeof(listPkt));

	// 방 생성 → JoinResult 에서 InRoom 으로
	C2S_RoomCreatePacket createPkt;
	SendTcp(&createPkt, sizeof(createPkt));
	SetState(ClientState::CreatingRoom);
}

void VirtualClient::SendRoomReady()
{
	C2S_RoomReadyPacket readyPkt;
	readyPkt.roomId = mRoomId;
	readyPkt.ready = 1;
	SendTcp(&readyPkt, sizeof(readyPkt));
	mReadySent = true;
}

void VirtualClient::SendSceneChangeToGame()
{
	C2S_SceneChangePacket scenePkt(SceneId::FirstGame);
	SendTcp(&scenePkt, sizeof(scenePkt));
	SetState(ClientState::SceneChanging);
}

void VirtualClient::SendGameStart()
{
	C2S_StartGamePacket startPkt(mClientId, mRoomPlayerType);
	startPkt.SessionId = mClientId;
	SendUdp(&startPkt, sizeof(startPkt));
	mRoomActionTimer = 1.0f;   // 다음 재전송까지 1s 대기
	SetState(ClientState::GameStarting);
}

// ─── 패킷 수신 핸들러 ─────────────────────────────────────────
void VirtualClient::OnJoinResult(const S2C_RoomJoinResultPacket& pkt)
{
	if (mState != ClientState::CreatingRoom && mState != ClientState::JoiningRoom)
		return;

	if (pkt.success == 0)
	{
		// 입장/생성 실패. LobbyChurn 은 다시 홀로, FullGame 은 실패 처리
		if (mScenario == TestScenario::LobbyChurn)
		{
			mRoomId = 0;
			mChurnTimer = 0.5f;
			SetState(ClientState::InLobby);
		}
		else
		{
			SetState(ClientState::Failed);
		}
		return;
	}

	mRoomId = pkt.roomId;

	// Host 는 발급받은 roomId 를 그룹에 publish → Member 들이 join
	if (mScenario == TestScenario::FullGame && mIsRoomHost && mManager)
		mManager->PublishGroupRoomId(mRoomGroup, mRoomId);

	mReadySent = false;
	SetState(ClientState::InRoom);
}

void VirtualClient::OnRoomState(const S2C_RoomStatePacket& pkt)
{
	if (pkt.roomId != mRoomId)
		return;

	// 내 캐릭터 타입 갱신 + 전원 Ready 여부 계산
	int readyCount = 0;
	int pc = pkt.playerCount;
	for (int i = 0; i < pc && i < ROOM_MAX_PLAYERS; ++i)
	{
		if (pkt.slots[i].sessionId == mClientId)
			mRoomPlayerType = pkt.slots[i].playerType;
		if (pkt.slots[i].ready)
			++readyCount;
	}

	mRoomAllReady = (pc > 0) && (readyCount == pc) && (pc >= mExpectedRoomSize);
}

void VirtualClient::OnSceneChangeResult(const S2C_SceneChangeResultPacket& pkt)
{
	if (pkt.approved == 0)
	{
		// 게임 시작 거부(자격 미달 등). FullGame 에서만 의미
		if (mState == ClientState::SceneChanging || mState == ClientState::Ready)
		{
			// 재시도 여지를 위해 Ready 로 복귀
			SetState(ClientState::Ready);
		}
		return;
	}

	if (pkt.currentScene == SceneId::FirstGame)
	{
		// 씬 전환 승인 → 게임 시작(스폰) 요청
		if (mState == ClientState::Ready || mState == ClientState::SceneChanging)
			SendGameStart();
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
	// 한 틱에 대기 중인 UDP 데이터그램을 모두 비운다(WOULDBLOCK까지).
	// 틱당 1개만 읽으면 부하 시 OS 버퍼가 넘쳐 대부분 드롭 → 수신/SrvHz 측정 왜곡.
	for (;;) {
		sockaddr_in fromAddr{};
		int fromLen = sizeof(fromAddr);
		int len = recvfrom(mUdpSocket, (char*)mUdpRecvBuffer, sizeof(mUdpRecvBuffer), 0,
			(sockaddr*)&fromAddr, &fromLen);
		if (len > 0) {
			mBytesRecv += len;
			mPacketsRecv++;
			ProcessUdpPacket(mUdpRecvBuffer, len);
		}
		else {
			break; // WOULDBLOCK 또는 오류 → 비우기 종료
		}
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
		if (mState == ClientState::TcpConnected)
			SetState(ClientState::Registering);
		break;
	}
	case PKT_Type::S2C_PKT_LOGIN:
	{
		S2C_LoginPacket pkt;
		::memcpy(&pkt, buffer, sizeof(S2C_LoginPacket));
		mClientId = pkt.clientId;
		if (mState == ClientState::TcpConnected)
			SetState(ClientState::Registering);
		break;
	}
	case PKT_Type::S2C_ROOM_JOIN_RESULT:
	{
		if (size >= sizeof(S2C_RoomJoinResultPacket)) {
			S2C_RoomJoinResultPacket pkt;
			::memcpy(&pkt, buffer, sizeof(S2C_RoomJoinResultPacket));
			OnJoinResult(pkt);
		}
		break;
	}
	case PKT_Type::S2C_ROOM_STATE:
	{
		if (size >= sizeof(S2C_RoomStatePacket)) {
			S2C_RoomStatePacket pkt;
			::memcpy(&pkt, buffer, sizeof(S2C_RoomStatePacket));
			OnRoomState(pkt);
		}
		break;
	}
	case PKT_Type::S2C_SCENE_CHANGE_RESULT:
	{
		if (size >= sizeof(S2C_SceneChangeResultPacket)) {
			S2C_SceneChangeResultPacket pkt;
			::memcpy(&pkt, buffer, sizeof(S2C_SceneChangeResultPacket));
			OnSceneChangeResult(pkt);
		}
		break;
	}
	case PKT_Type::S2C_PKT_SPAWN:
	{
		// 서버는 S2C_GAME_START 를 보내지 않는다. 본인 스폰(isLocalPlayer=1)이 게임 진입 확인.
		if (size >= sizeof(S2C_SpawnPacekt)) {
			S2C_SpawnPacekt pkt;
			::memcpy(&pkt, buffer, sizeof(S2C_SpawnPacekt));
			if (pkt.isLocalPlayer == 1 || pkt.SessionId == mClientId) {
				mNetEntityId = pkt.netEntityId;  // 이동/액션에 쓸 서버 netId
				if (mState == ClientState::GameStarting)
					SetState(ClientState::Playing);
			}
		}
		break;
	}
	case PKT_Type::S2C_GAME_START:
	{
		// (서버가 보내지 않지만 호환을 위해 유지)
		if (mState == ClientState::GameStarting)
			SetState(ClientState::Playing);
		break;
	}
	case PKT_Type::S2C_PKT_SYNC:
	{
		// Sync echo: clientEchoTime 은 우리가 보낸 SendTime. RTT = now - echo.
		if (size >= sizeof(S2C_SyncPacket)) {
			S2C_SyncPacket pkt;
			::memcpy(&pkt, buffer, sizeof(S2C_SyncPacket));
			double rttMs = (NowSeconds() - pkt.clientEchoTime) * 1000.0;
			if (rttMs >= 0.0 && rttMs < 60000.0) { // 비정상값 가드
				mLastRttMs = rttMs;
				mRttSumMs += rttMs;
				mRttCount++;
				if (rttMs > mRttMaxMs) mRttMaxMs = rttMs;
			}
		}
		break;
	}
	// Other S2C packets - just consume (게임 중 수신 트래픽)
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
	{
		// 서버 브로드캐스트 틱 관측: Sequence(=서버 sendTick)가 전진할 때마다 1 카운트.
		// 한 SendMove 의 모든 엔티티 패킷은 같은 Sequence라 틱당 1회만 증가.
		if (size >= sizeof(PacketUdpHeader)) {
			PacketUdpHeader uh;
			::memcpy(&uh, buffer, sizeof(PacketUdpHeader));
			if (uh.Sequence > mLastSrvSeq) {
				if (mLastSrvSeq != 0) mSrvTickCount++;
				mLastSrvSeq = uh.Sequence;
			}
		}
		break;
	}
	case PKT_Type::S2C_GAME_START:
		if (mState == ClientState::GameStarting)
			SetState(ClientState::Playing);
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

	// 서버가 부여한 netId 사용(없으면 clientId 폴백). RecvInput 의 netId 일치 검사 통과용.
	const uint64 netId = (mNetEntityId != 0) ? mNetEntityId : mClientId;

	// Send move packet
	mMoveTimer -= dt;
	if (mMoveTimer <= 0) {
		C2S_MovePacket movePkt;
		movePkt.netEntityId = netId;
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
		actionPkt.netEntityId = netId;
		actionPkt.Buttons = 1;
		actionPkt.Yaw = mYaw;
		actionPkt.Pitch = 0;
		SendTcp(&actionPkt, sizeof(actionPkt));
		mActionTimer = mActionInterval;
	}

	// RTT 측정용 Sync ping (1초마다). 서버는 게임 월드(BeatSystem)에서만 echo 응답.
	mSyncTimer -= dt;
	if (mSyncTimer <= 0) {
		C2S_SyncPacket syncPkt;
		syncPkt.clientId = mClientId;
		syncPkt.SendTime = NowSeconds();   // 서버가 clientEchoTime 으로 그대로 echo
		SendTcp(&syncPkt, sizeof(syncPkt));
		mSyncTimer = 1.0f;
	}
}
