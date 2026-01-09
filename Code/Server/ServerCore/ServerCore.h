#pragma once
#include "NetAddress.h"



extern SpscRingQueue<SendRequest, 128 * 1024>							gSendQueue;
extern SpscRingQueue<InputCommand, 128 * 1024>							gRecvQueue;

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
	std::shared_ptr<class NetworkThread>				mNetworkThread;
};

