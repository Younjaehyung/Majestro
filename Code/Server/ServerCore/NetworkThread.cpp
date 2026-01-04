#include "pch.h"
#include "NetworkThread.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "SendBuffer.h"
#include "Session.h"



NetworkThread::NetworkThread()
{
	mListenSocket = INVALID_SOCKET;
    mSessionMgr.Initialize();
    SendBufferManager::Initialize(1000);
}

NetworkThread::NetworkThread(SOCKET listenSocket)
    : mListenSocket(listenSocket)
{
}

NetworkThread::~NetworkThread()
{
	Stop();
}

void NetworkThread::Start()
{
	
	mThread = std::thread([this]()
	{
		mRunning = true;
		while (mRunning)
		{
			Update();
		}
	});
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
    while (mRunning)
    {
        fd_set readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(mListenSocket, &readSet);


        for (auto& s : mSessionMgr.GetAllSessions())
        {   
            FD_SET(s.second->GetSocket(), &readSet);

            if (Send())
                FD_SET(s.second->GetSocket(), &writeSet);
        }

        timeval tv{ 0, 1000 }; // 1ms
        select(0, &readSet, &writeSet, nullptr, &tv);

        if (FD_ISSET(mListenSocket, &readSet))
            AcceptClient();

        for (auto& s : mSessionMgr.GetAllSessions())
        {
            if (s.second->IsConnected() == false)
                continue;

            if (FD_ISSET(s.second->GetSocket(), &readSet))
                HandleRecv(const_cast<std::shared_ptr<Session>&>(s.second));

            if (FD_ISSET(s.second->GetSocket(), &writeSet))
                HandleSend(const_cast<std::shared_ptr<Session>&>(s.second));
        }

        CleanupDisconnected();
    }
}



void NetworkThread::AcceptClient()
{
    sockaddr_in clientaddr;
    int32 addrlen = sizeof(clientaddr);
    SOCKET s = accept(mListenSocket, (struct sockaddr*)&clientaddr, &addrlen);
    u_long one = 1;
    ioctlsocket(s, FIONBIO, &one);

    shared_ptr<Session> session = mSessionMgr.CreateSessions(s);
	mSessionMgr.AddSession(session);

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
				LOG_INFO("WSAEWOULDBLOCK on Send, ID:[{}]", session->GetPlayerId());
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
    for (auto& s : mSessionMgr.GetAllSessions())
    {
        std::lock_guard<std::mutex> lock(s.second->mMutex);
        if (s.second->GetConnectedAtomic()) continue;

        s.second->Close();
        mSessionMgr.RemoveSession(s.second);
    }

}

bool NetworkThread::Send()
{
  
    if (gSendQueue.Empty())
        return false;

	gSendQueue.Pop(mData);
   
    mSessionMgr.mSessions[mData.SessionId]->SendData(mData.Data, mData.Len);


    return true;
}

void NetworkThread::BroadcastPacket(SendRequest& pkt)
{
    if (pkt.SessionId != 0) {
        LOG_ERROR("BroadcastPacket SessionId is 0");
        return;
    }

    for (auto& s : mSessionMgr.GetAllSessions())
    {
		pkt.SessionId = s.second->GetPlayerId();
		gSendQueue.Push(pkt);
    }
}
