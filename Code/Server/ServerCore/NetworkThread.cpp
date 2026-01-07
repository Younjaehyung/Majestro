#include "pch.h"
#include "NetworkThread.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "SendBuffer.h"
#include "SocketUtils.h"
#include "Session.h"



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
    

    // 소켓 생성
    mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenSocket == INVALID_SOCKET) LOG_ERROR("err(socket)");

    u_long on = 1;
    if (::ioctlsocket(mListenSocket, FIONBIO, &on) == INVALID_SOCKET) {
        LOG_ERROR("err(ioct)");
        return;
    }
        

    // bind()
    if (false == SocketUtils::BindAnyAddress(mListenSocket, 9000)) {
        LOG_ERROR("err(bind)");
        SocketUtils::Close(mListenSocket);
        SocketUtils::Clear();
        return;
    }

    // listen()
    if (false == SocketUtils::Listen(mListenSocket, SOMAXCONN)) {
        LOG_ERROR("err(listen)");
        SocketUtils::Close(mListenSocket);
        SocketUtils::Clear();
        return;
    }

    LOG_INFO("START GAME SERVER");
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

        
        Send();

        for (auto& s : mSessionMgr.mSessions)
        {
			if (s.second == nullptr) continue;
			if (s.second->IsConnected() == false) continue;

            FD_SET(s.second->GetSocket(), &readSet);

            if (!s.second->mSendBufferQueue.empty())
                FD_SET(s.second->GetSocket(), &writeSet);
        }

        timeval tv{ 0, 1000 }; // 1ms
        int32 ready = select(0, &readSet, &writeSet, nullptr, &tv);
        if (ready <= 0)
            continue;

        if (FD_ISSET(mListenSocket, &readSet))
            AcceptClient();

        for (auto& s : mSessionMgr.mSessions)
        {
            if (s.second == nullptr) continue;
            if (s.second->IsConnected() == false) continue;

            if (FD_ISSET(s.second->GetSocket(), &readSet))
                HandleRecv(s.second);

            if (FD_ISSET(s.second->GetSocket(), &writeSet))
                HandleSend(s.second);
        }

        CleanupDisconnected();

    }
}



void NetworkThread::AcceptClient()
{
    sockaddr_in clientaddr;
    int32 addrlen = sizeof(clientaddr);
    SOCKET s = accept(mListenSocket, (struct sockaddr*)&clientaddr, &addrlen);
    if(s == INVALID_SOCKET)
    {
        int32 error = WSAGetLastError();
        LOG_ERROR("Accept Failed, error code : {}", error);
        return;
	}

    shared_ptr<Session> session = mSessionMgr.CreateSessions(s);

	LOG_INFO("New Client Connected: [{}], Client IP : {}, Port : [{}]", 
        session->GetPlayerId(), session->GetAddress().GetIpAddressA(),
        session->GetAddress().GetPort());

	SendBuffer* sendBuffer = SendBufferManager::Acquire();
	KLoginPacket loginPkt = KLoginPacket(session->GetPlayerId());
	sendBuffer->SetData(&loginPkt, sizeof(KLoginPacket));
    session->mSendBufferQueue.push(sendBuffer);
}

void NetworkThread::HandleRecv(std::shared_ptr<Session>& session)
{
    session->mRecvBuffer.Clean();

    if (session->mRecvBuffer.FreeSize() <= 0)
    {
        session->Disconnect("RecvBuffer Full");
        return;
    }

	int len = recv(session->GetSocket(), (char*)session->mRecvBuffer.WritePos(),
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
            int32 ret = session->OnRecv(session->mRecvBuffer.ReadPos(), session->mRecvBuffer.DataSize());

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

void NetworkThread::HandleSend(std::shared_ptr<Session>& session)
{
    if (session->mSendBufferQueue.empty())
        return;


    LOG_INFO("HandleSend ID:[{}] SendBufferQueue Size:[{}]",
        session->GetPlayerId(), session->mSendBufferQueue.size());

    while (!session->mSendBufferQueue.empty())
    {
		SendBuffer* sb = session->mSendBufferQueue.front();

        // 아직 보내야 할 바이트 수
        uint32 remain = sb->Capacity - sb->ReadPos;

        int len = send(session->GetSocket(), (char*)(sb->Data+sb->ReadPos),
            remain, 0);

        if (len < 0)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
            {
                // 지금은 더 못 보냄 → 다음 select 때 재시도
				LOG_INFO("WSAEWOULDBLOCK on Send, ID:[{}]", session->GetPlayerId());
                break;
            }

            // 치명적 오류
            session->Disconnect("Send Error");
            return;
        }

        if (len == 0)
        {
            // TCP에서 send 0은 거의 없음 → 안전하게 종료
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
        session->mSendBufferQueue.pop();
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
        // SessionManager가 내부적으로 map을 지우는 경우 이터레이터가 더 위험해질 수 있으므로,
        // 여기서는 직접 지우는 편이 안전합니다.
        it = mSessionMgr.mSessions.erase(it);
    }
}
bool NetworkThread::Send()
{

    while (gSendQueue.Pop(mData)) {


        SendBuffer* sendBuffer = SendBufferManager::Acquire();
        if (sendBuffer == nullptr)
        {
            LOG_ERROR("SendBuffer Acquire Failed");
            continue;
        }

        if (false == SendRequestPacket::SerializePacket(mData, sendBuffer)) {
            SendBufferManager::Release(sendBuffer);
            continue;
        }

        if (mData.SessionId == 0)
        {
            for (auto& s : mSessionMgr.mSessions  ) {
                s.second->SendData(sendBuffer);
            }
        }
        else if (mSessionMgr.mSessions.contains(mData.SessionId))
        {
            mSessionMgr.mSessions[mData.SessionId]->SendData(sendBuffer);
        }
        else
        {
            SendBufferManager::Release(sendBuffer);
        }
    }
    return true;
}

