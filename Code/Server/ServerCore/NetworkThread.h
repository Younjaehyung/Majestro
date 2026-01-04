#pragma once
#include <thread>
#include <atomic>
#include "PacketHelper.h"

class ServerCore;

class NetworkThread
{
public:
	NetworkThread();
	NetworkThread(SOCKET listenSocket);
	~NetworkThread();
	void Start();
	void Update();
	void Stop();

public:
	void									SetServerCore(std::weak_ptr<ServerCore> serverCore) { mServerCore = serverCore; }
	std::weak_ptr<ServerCore>				GetServerCore() { return mServerCore; }

public:
	void AcceptClient();
	void HandleRecv(std::shared_ptr<class Session>& session);
	void HandleSend(std::shared_ptr<class Session>& session);
	void CleanupDisconnected();
	void BroadcastPacket(SendRequest& pkt);
public:		// game logic thread 와의 통신용
	bool Send();
	bool Recv(InputCommand& pkt);

private:
	std::thread mThread;
	SOCKET		mListenSocket = INVALID_SOCKET;
	bool		mRunning = false;

	std::weak_ptr<ServerCore>						mServerCore;
	SessionManager									mSessionMgr;
private:
	SendRequest mData;
};

