#pragma once


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

private:

private:
	std::shared_ptr<class NetworkThread>				mNetworkThread;
};

