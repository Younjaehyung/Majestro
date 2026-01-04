#pragma once
#include "NetAddress.h"

extern class SessionManager gSessionMgr;
extern SpscRingQueue<SendRequest, 128>							gSendQueue;
extern SpscRingQueue<InputCommand, 128>							gRecvQueue;

class ServerCore
{
public:
	ServerCore();
	~ServerCore();


	void Initialize();
	void Start();
	void Update();
	void Stop();

	void BroadcastPacket(SendRequest& pkt);
	void UnicastPacket(SendRequest& pkt);
private:

private:
	SOCKET												mListenSocket;
	std::shared_ptr<class NetworkThread>				mNetworkThread;
};

