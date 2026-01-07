#pragma once
#include <thread>
#include <atomic>
#include "SessionManager.h"
#include "PacketHelper.h"

class ServerCore;



class NetworkThread
{
public:
	NetworkThread();
	NetworkThread(SOCKET listenSocket);
	~NetworkThread();

	void Initialize();

	void Start();
	void Update();
	void Stop();

public:
	void									SetServerCore(std::weak_ptr<ServerCore> serverCore) { mServerCore = serverCore; }
	std::weak_ptr<ServerCore>				GetServerCore() { return mServerCore; }

public:
	void AcceptClient();
	void HandleTcpRecv(std::shared_ptr<class Session>& session);
	void HandleUdpRecv(std::shared_ptr<class Session>& session);
	void HandleTcpSend(std::shared_ptr<class Session>& session);
	void HandleUdpSend(std::shared_ptr<class Session>& session);
	void CleanupDisconnected();
public:		// game logic thread 와의 통신용
	bool Send();

private:
	std::thread mThread;
	SOCKET		mListenSocket = INVALID_SOCKET;
	atomic<bool>		mRunning = false;

	std::weak_ptr<ServerCore>						mServerCore;
	SessionManager									mSessionMgr;
private:
	SendRequest mData;
};

