#pragma once
#include <thread>
#include <atomic>
#include <mutex>

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
private:
	std::thread mThread;
	SOCKET		mListenSocket = INVALID_SOCKET;
	bool		mRunning = false;

	std::weak_ptr<ServerCore>						mServerCore;
};

