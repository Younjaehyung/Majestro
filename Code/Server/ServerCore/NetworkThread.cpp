#include "pch.h"
#include "NetworkThread.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "SendBuffer.h"
#include "SocketUtils.h"
#include "Session.h"
#include <fstream>

// NetworkConfig.json 에서 리슨 포트를 읽기
// 서버는 INADDR_ANY 로 바인드하므로 serverIp 필드는 사용 X
static int LoadServerPort(int defaultPort)
{
    const char* path = "../Resources/Json/NetworkConfig.json";
    try
    {
        std::ifstream file(path);
        if (!file.is_open())
        {   // 파일이 없거나 파싱에 실패하면 기본 포트(9000)를 사용
            MJLOG_WARN(Startup, "NetworkConfig.json 없음 -> 기본 포트 {} 사용", defaultPort);
            return defaultPort;
        }

        json config;
        file >> config;

        if (config.contains("port") && config["port"].is_number_integer())
        {
            int port = config["port"].get<int>();
            MJLOG_INFO(Startup, "NetworkConfig 로드: TCP {}, UDP {}", port, port + 1);
            return port;
        }
    }
    catch (const std::exception& e)
    {
        MJLOG_ERROR(Startup, "NetworkConfig 파싱 실패({}) -> 기본 포트 {} 사용", e.what(), defaultPort);
    }
    return defaultPort;
}


NetworkThread::NetworkThread()
{
	mListenSocket = INVALID_SOCKET;
    Initialize();
}

NetworkThread::NetworkThread(SOCKET listenSocket)
    : mListenSocket(listenSocket)
{
    Initialize();
}

NetworkThread::~NetworkThread()
{
	Stop();
}



void NetworkThread::Initialize()
{
    mSessionMgr.Initialize();

    // 리슨 포트 결정 (config 우선, 실패 시 기본 9000)
    const int tcpPort = LoadServerPort(9000);
    const int udpPort = tcpPort + 1;

    // 소켓 생성
    mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenSocket == INVALID_SOCKET)
        MJLOG_ERROR(Startup, "TCP listen 소켓 생성 실패 code={}", WSAGetLastError());

    mUdpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (mUdpSock == INVALID_SOCKET) {
        int32 error = WSAGetLastError();
        MJLOG_ERROR(Startup, "UDP 소켓 생성 실패 code={}", error);
        return;
    }

    sockaddr_in udpAddr{};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpAddr.sin_port = htons(udpPort);   // 클라가 보내는 서버 UDP 포트

    if (::bind(mUdpSock, (sockaddr*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
        int32 error = WSAGetLastError();
        MJLOG_ERROR(Startup, "UDP bind 실패 port={} code={}", udpPort, error);
        return;
    }
    u_long on1 = 1;
    if (::ioctlsocket(mUdpSock, FIONBIO, &on1) == INVALID_SOCKET) {
        MJLOG_ERROR(Startup, "UDP 논블로킹 전환 실패 code={}", WSAGetLastError());
        return;
    }

    u_long on = 1;
    if (::ioctlsocket(mListenSocket, FIONBIO, &on) == INVALID_SOCKET) {
        MJLOG_ERROR(Startup, "TCP 논블로킹 전환 실패 code={}", WSAGetLastError());
        return;
    }
        

    // bind()
    if (false == SocketUtils::BindAnyAddress(mListenSocket, tcpPort)) {
        MJLOG_ERROR(Startup, "TCP bind 실패 port={}", tcpPort);
        SocketUtils::Close(mListenSocket);
        SocketUtils::Clear();
        return;
    }


    // listen()
    if (false == SocketUtils::Listen(mListenSocket, SOMAXCONN)) {
        MJLOG_ERROR(Startup, "TCP listen 실패 port={}", tcpPort);
        SocketUtils::Close(mListenSocket);
        SocketUtils::Clear();
        return;
    }

    MJLOG_INFO(Startup, "게임 서버 시작 TCP={} UDP={}", tcpPort, udpPort);
}

void NetworkThread::Start()
{
	
	mRunning = true;
    mThread = std::thread(&NetworkThread::Update, this);
    
}

void NetworkThread::Stop()
{
    mRunning = false;
    if (mThread.joinable())
    {
        mThread.join();
    }
    SendBufferManager::Shutdown();
    mSessionMgr.ClearSessions();
}

void NetworkThread::Update()
{
    while (mRunning) {


        fd_set readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(mListenSocket, &readSet);
        FD_SET(mUdpSock, &readSet);
        
        PushSend();

        for (auto& s : mSessionMgr.mSessions)
        {
			if (s.second == nullptr) continue;
			if (s.second->IsConnected() == false) continue;

            FD_SET(s.second->GetTSocket(), &readSet);
            

            if (!s.second->mTSendBufferQueue.empty()) {
                FD_SET(s.second->GetTSocket(), &writeSet);
            }
        }

        timeval tv{ 0, 1000 }; // 1ms
        int32 ready = select(0, &readSet, &writeSet, nullptr, &tv);
        for (auto& s : mSessionMgr.mSessions)
        {
            HandleUdpSend(s.second);
        }
        

        if (ready <= 0)
            continue;

        if (FD_ISSET(mListenSocket, &readSet))
            AcceptClient();

        
        if (FD_ISSET(mUdpSock, &readSet))
            HandleUdpRecv(); // 개별 세션 함수가 아닌 전체 수신 함수 호출

        for (auto& s : mSessionMgr.mSessions)
        {
            if (s.second == nullptr) continue;
            if (s.second->IsConnected() == false) continue;

            if (FD_ISSET(s.second->GetTSocket(), &readSet))
                HandleTcpRecv(s.second);

            if (FD_ISSET(s.second->GetTSocket(), &writeSet))
                HandleTcpSend(s.second);
  
			
        }

        CleanupDisconnected();

    }


	MJLOG_INFO(Startup, "네트워크 스레드 종료");
	static_assert(true);
}



void NetworkThread::AcceptClient()
{
    sockaddr_in clientaddr;
    int32 addrlen = sizeof(clientaddr);
    SOCKET tcpSock = accept(mListenSocket, (struct sockaddr*)&clientaddr, &addrlen);
    if(tcpSock == INVALID_SOCKET)
    {
        int32 error = WSAGetLastError();
        MJLOG_EVERY(Session, Warn, "accept-failed", 5.0,
            "accept 실패 code={}", error);
        return;
	}

    shared_ptr<Session> session = mSessionMgr.CreateSessions(tcpSock, mUdpSock);

	MJLOG_INFO(Session, "접속 id={} addr={}:{}",
        session->GetPlayerId(), session->GetTcpAddress().GetIpAddressA(),
        session->GetTcpAddress().GetPort());

	SendBuffer* sendBuffer = SendBufferManager::Acquire();
    S2C_LoginPacket loginPkt = S2C_LoginPacket(session->GetPlayerId());

	sendBuffer->SetData(&loginPkt, sizeof(S2C_LoginPacket),TCP);
    session->mTSendBufferQueue.push(sendBuffer);
    gNewSessions.Push(session->GetPlayerId());

   /* sendBuffer = SendBufferManager::Acquire();
	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(session->GetPlayerId(), , PrefabType::PLAYER);
    sendBuffer->SetData(&spawnPkt, sizeof(S2C_SpawnPacekt), TCP);
	session->mTSendBufferQueue.push(sendBuffer);*/
}

void NetworkThread::HandleTcpRecv(std::shared_ptr<Session>& session)
{
    session->mRecvBuffer.Clean();

    if (session->mRecvBuffer.FreeSize() <= 0)
    {
        session->Disconnect("RecvBuffer Full");
        return;
    }

	int len = recv(session->GetTSocket(), (char*)session->mRecvBuffer.WritePos(),
        session->mRecvBuffer.FreeSize(), 0);

	if (len > 0)
    {
        if (!session->mRecvBuffer.OnWrite(len))
        {
                session->Disconnect("RecvBuffer OnWrite Error");
                return;
        }

        while (true)
        {
            int32 ret = session->OnTcpRecv(session->mRecvBuffer.ReadPos(), session->mRecvBuffer.DataSize());

            if (ret < 0 || ret > session->mRecvBuffer.DataSize())
            {
                session->Disconnect("OnRecv Error");
                return;
            }
            else if (ret == 0)
            {
                break;
            }
            session->mRecvBuffer.OnRead(ret);
        }
    }
    else if (len == 0)
    {
        session->Disconnect("Recv 0");
    }
    else
    {
        int32 errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK)
        {
            session->Disconnect("Recv Error");
        }
	}
}

void NetworkThread::HandleUdpRecv()
{
	

    sockaddr_in fromAddr{};
    int fromLen = sizeof(fromAddr);

    // UDP 소켓에 쌓인 모든 데이터를 비움
    while (true) {
        int len = ::recvfrom(mUdpSock, (char*)mURecvBuffer, BUFSIZE, 0, (sockaddr*)&fromAddr, &fromLen);

        if (len <= 0) break;

        if (len < (int)sizeof(PacketHeader))
            continue;

        auto session = mSessionMgr.FindSessionByAddr(fromAddr);
        if (session) {
            session->OnUdpRecv(mURecvBuffer, len);
           
            continue;
        }
       

        PacketHeader* header = (PacketHeader*)mURecvBuffer;
        if (header->PacketType == C2S_PKT_LOGIN || header->PacketType == C2S_GAME_START) {
            uint32 clientId = 0;
            uint32 sessionId = 0;
            if (header->PacketType == C2S_PKT_LOGIN) {
                const C2S_LoginPacket* loginPkt = (C2S_LoginPacket*)mURecvBuffer;
                clientId = loginPkt->clientId;
                sessionId = loginPkt->SessionId;
            }
            else {
                const C2S_StartGamePacket* startPkt = (C2S_StartGamePacket*)mURecvBuffer;
                clientId = startPkt->clientId;
                sessionId = startPkt->SessionId;
            }
            const uint32 lookupId = (clientId != 0) ? clientId : sessionId;
            auto targetIt = mSessionMgr.mSessions.find(lookupId);
            if (targetIt != mSessionMgr.mSessions.end() && targetIt->second /*&& targetIt->second->VerifyToken(pkt->token)*/) {
                // 주소 매핑 등록
                targetIt->second->SetUNetAddress(fromAddr);
                mSessionMgr.RegisterUdpAddress(fromAddr, clientId);
                targetIt->second->OnUdpRecv(mURecvBuffer, len);
            }
            else {
                MJLOG_EVERY(Session, Warn,
                    std::to_string(clientId) + ":" + std::to_string(ntohs(fromAddr.sin_port)),
                    5.0, "주인 없는 UDP 패킷 clientId={} sessionId={} port={}",
                    clientId, sessionId, ntohs(fromAddr.sin_port));
            }


        }

 //       if (header->PacketType == C2S_PKT_LOGIN) {
 //           C2S_LoginPacket* pkt = (C2S_LoginPacket*)mURecvBuffer;

 //           auto& targetSession = mSessionMgr.mSessions[pkt->clientId];
 //           if (targetSession /*&& targetSession->VerifyToken(pkt->token)*/) {
 //               // 주소 매핑 등록
 //               targetSession->SetUNetAddress(fromAddr);
 //               mSessionMgr.RegisterUdpAddress(fromAddr, pkt->clientId);
 //               targetSession->OnUdpRecv(mURecvBuffer, len);
 //               
 //           }
 //           std::cout << "No session found for UDP packet from " << std::endl;
 //           std::cout<<targetSession->GetUdpAddress().GetPort() << std::endl;
 //           
	//		// client에게 prefab생성 패킷 전송
	///*		SendBuffer* sendBuffer = SendBufferManager::Acquire();
	//		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(targetSession->GetPlayerId(), targetSession->GetPlayerId(), PrefabType::PLAYER);
	//		sendBuffer->SetData(&spawnPkt, sizeof(S2C_SpawnPacekt), TCP);
	//		targetSession->mTSendBufferQueue.push(sendBuffer);
	//		*/
 //       }

        

    }
    
}

void NetworkThread::HandleTcpSend(std::shared_ptr<Session>& session)
{
    if (session->mTSendBufferQueue.empty())
        return;


    while (!session->mTSendBufferQueue.empty())
    {
		SendBuffer* sb = session->mTSendBufferQueue.front();

        // 아직 보내야 할 바이트 수
        uint32 remain = sb->Capacity - sb->ReadPos;

        int len = send(session->GetTSocket(), (char*)(sb->Data+sb->ReadPos),
            remain, 0);

        if (len < 0)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
            {
				MJLOG_EVERY(PacketFlow, Info,
					"wouldblock:" + std::to_string(session->GetPlayerId()), 5.0,
					"송신 대기(WSAEWOULDBLOCK) id={}", session->GetPlayerId());
                break;
            }

            // 치명적 오류
            session->Disconnect("Send Error");
            return;
        }

        if (len == 0)
        {
            session->Disconnect("Send 0");
            return;
        }

        // 부분/전체 전송 반영
        sb->ReadPos += len;

        if (sb->ReadPos > sb->Capacity)
        {
            session->Disconnect("BUFFER ERROR");
            SendBufferManager::Release(sb);
            return;
        }
        if (sb->Capacity > sb->ReadPos)
        {
			continue; // 아직 덜 보냄
        }

        // 전송 완료
        session->mTSendBufferQueue.pop();
        SendBufferManager::Release(sb);

    }


}

void NetworkThread::HandleUdpSend(std::shared_ptr<class Session>& session)
{
    while (!session->mUSendBufferQueue.empty()) {
        sockaddr_in to = session->GetUdpAddress().GetSockAddr();
        if (to.sin_port == 0) return;
        SendBuffer* sb = session->mUSendBufferQueue.front();
		
        int len = sendto(mUdpSock, (char*)sb->Data, sb->Capacity, 0,
            (sockaddr*)&to, sizeof(sockaddr_in));

        if (len == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
            {
                // 지금은 못 보냄: reliable이면 unacked에 남겨두고 다음 틱에 시도

                break;
            }

            return;
        }

        session->mUSendBufferQueue.pop();
        SendBufferManager::Release(sb);
    }
}
        


void NetworkThread::CleanupDisconnected()
{
    for (auto it = mSessionMgr.mSessions.begin(); it != mSessionMgr.mSessions.end(); )
    {
        const auto& session = it->second;
        if (session == nullptr)
        {
            it = mSessionMgr.mSessions.erase(it);
            continue;
        }

        if (session->IsConnected())
        {
            ++it;
            continue;
        }

        session->Close();

        // 로직 스레드에 세션 종료를 통지(내부 신호)
        InputCommand leave{};
        leave.SessionId = session->GetPlayerId();
        leave.Type = PKT_Type::INTERNAL_SESSION_LEAVE;
        gRecvQueue.Push(leave);

        it = mSessionMgr.mSessions.erase(it);
    }
}
bool NetworkThread::PushSend()
{

    while (gSendQueue.Pop(mData)) {



        if (mData.SessionId == 0)
        {


            for (auto& s : mSessionMgr.mSessions  ) {
            
                SendBuffer* sendBuffer = SendBufferManager::Acquire();
                if (sendBuffer == nullptr)
                {
                    MJLOG_EVERY(Session, Error, "sendbuffer-exhausted-broadcast", 5.0,
                        "SendBuffer 고갈 (브로드캐스트)");
                    continue;
                }

                if (false == SendRequestPacket::SerializePacket(mData, sendBuffer)) {
                    SendBufferManager::Release(sendBuffer);
                    continue;
                }

                switch (sendBuffer->Protocol)
                {
                case TCP:

                    s.second->SendTcpData(sendBuffer);
                    break;
                case UDP:

                    s.second->SendUdpData(sendBuffer);
                    break;
                default:
                    SendBufferManager::Release(sendBuffer);
                    break;
                }
            }
        }
        else if (mSessionMgr.mSessions.contains(mData.SessionId))
        {
            SendBuffer* sendBuffer = SendBufferManager::Acquire();
            if (sendBuffer == nullptr)
            {
                MJLOG_EVERY(Session, Error, "sendbuffer-exhausted-unicast", 5.0,
                    "SendBuffer 고갈 (유니캐스트) session={}", mData.SessionId);
                continue;
            }

            if (false == SendRequestPacket::SerializePacket(mData, sendBuffer)) {
                SendBufferManager::Release(sendBuffer);
                continue;
            }

            switch (sendBuffer->Protocol)
            {
            case TCP:
                
                mSessionMgr.mSessions[mData.SessionId]->SendTcpData(sendBuffer);
                break;
            case UDP:
                
                mSessionMgr.mSessions[mData.SessionId]->SendUdpData(sendBuffer);
                break;
            default:
                SendBufferManager::Release(sendBuffer);
                break;
            }

           
        }

    }
    return true;
}

