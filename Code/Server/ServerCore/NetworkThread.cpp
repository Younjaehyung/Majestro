#include "pch.h"
#include "NetworkThread.h"
#include "ServerCore.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "SendBuffer.h"
#include "Session.h"

NetworkThread::NetworkThread()
{
}

NetworkThread::~NetworkThread()
{
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

      //  CleanupDisconnected();
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

    printf("player %d connected\n", session->GetPlayerId());
	
    printf("Client IP: %ls, Port: %d\n",
        session->GetAddress().GetIpAddress().c_str(),
		session->GetAddress().GetPort());
}

void NetworkThread::HandleRecv(std::shared_ptr<Session>& session)
{
	int len = recv(session->GetSocket(), (char*)session->mRecvBuffer.WritePos(),
        session->mRecvBuffer.FreeSize(), 0);

	if (len > 0)
        {
        session->mRecvBuffer.OnWrite(len);
        while (true)
        {
            int32 ret = session->OnRecv(session->mRecvBuffer.ReadPos(), session->mRecvBuffer.DataSize());
            if (ret < 0 || ret > session->mRecvBuffer.DataSize())
            {
                session->Disconnect(L"OnRecv Error");
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
        session->Disconnect(L"Recv 0");
    }
    else
    {
        int32 errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK)
        {
            session->Disconnect(L"Recv Error");
        }
	}
}

void NetworkThread::HandleSend(std::shared_ptr<Session>& session)
{

}
