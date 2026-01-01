#pragma once
#include "NetAddress.h"

extern class ThreadManager* GThreadManager;
extern class SessionManager gSessionMgr;

class ServerCore
{
public:
	ServerCore();
	~ServerCore();


	void Initialize();
	void Start();
	void Update();
	void Stop();
private:

private:
	SOCKET mListenSocket;
	std::shared_ptr<class NetworkThread>				mNetworkThread;
};

