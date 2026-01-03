#include "pch.h"
#include "NetworkThread.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "SendBuffer.h"
#include "Session.h"

NetworkThread::NetworkThread()
{
}

NetworkThread::NetworkThread(SOCKET listenSocket)
    : mListenSocket(listenSocket)
{
}

NetworkThread::~NetworkThread()
{
}

void NetworkThread::Start()
{
	SendBufferManager::Initialize(1000);
	mThread = std::thread([this]()
	{
		mRunning = true;
		while (mRunning)
		{
			Update();
		}
	});
}

void NetworkThread::Update()
{
    while (mRunning)
    {
        fd_set readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(mListenSocket, &readSet);


        for (auto& s : gSessionMgr.GetAllSessions())
        {
            FD_SET(s->GetSocket(), &readSet);

            //if (!s->mSendBuffer.empty())
            //    FD_SET(s->GetSocket(), &writeSet);
        }

        timeval tv{ 0, 1000 }; // 1ms
        select(0, &readSet, &writeSet, nullptr, &tv);

        if (FD_ISSET(mListenSocket, &readSet))
            AcceptClient();

        for (const std::shared_ptr<Session>& s : gSessionMgr.GetAllSessions())
        {
            if (FD_ISSET(s->GetSocket(), &readSet))
                HandleRecv(const_cast<std::shared_ptr<Session>&>(s));

            if (FD_ISSET(s->GetSocket(), &writeSet))
                HandleSend(const_cast<std::shared_ptr<Session>&>(s));
        }

        CleanupDisconnected();
    }
}

void NetworkThread::Stop()
{
	mRunning = false;
	if (mThread.joinable())
	{
		mThread.join();
	}
}

void NetworkThread::AcceptClient()
{
    sockaddr_in clientaddr;
    int32 addrlen = sizeof(clientaddr);
    SOCKET s = accept(mListenSocket, (struct sockaddr*)&clientaddr, &addrlen);
    u_long one = 1;
    ioctlsocket(s, FIONBIO, &one);

    shared_ptr<Session> session = gSessionMgr.CreateSessions(s);

	gSessionMgr.AddSession(session);

	LOG_INFO("New Client Connected: [{}], Client IP : {}, Port : [{}]", 
        session->GetPlayerId(), session->GetAddress().GetIpAddressA(),
        session->GetAddress().GetPort());

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
                return;
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
    for (const shared_ptr<Session>& s : gSessionMgr.GetAllSessions())
    {
        if (s->GetConnectedAtomic()) continue;

        s->Close();
        gSessionMgr.RemoveSession(s);
    }

}
