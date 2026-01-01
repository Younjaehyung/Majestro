#pragma once
#include <thread>
#include <atomic>
#include <mutex>



class NetworkThread
{
public:
	NetworkThread();
	~NetworkThread();
	void Start();
	void Update();
	void Stop();

public:
	void AcceptClient();
	void HandleRecv(std::shared_ptr<class Session>& session);
	void HandleSend(std::shared_ptr<class Session>& session);
private:
	std::thread mThread;
	SOCKET		mListenSocket = INVALID_SOCKET;


	bool		mRunning = false;

};

