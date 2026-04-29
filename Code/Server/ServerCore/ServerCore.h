#pragma once
#include "NetAddress.h"



extern SpscRingQueue<SendRequest, 128 * 1024>							gSendQueue;
extern SpscRingQueue<InputCommand, 128 * 1024>							gRecvQueue;
extern SpscRingQueue<uint32, 1024>										gNewSessions;

// 주기적 전송 타이머. hz로 초기화하면 Tick()이 true를 반환할 때만 전송하면 됨
struct RateLimiter
{
	float Interval;
	float Accumulator = 0.f;

	RateLimiter(float hz) : Interval(1.f / hz) {}

	bool Tick(float dt)
	{
		Accumulator += dt;
		if (Accumulator >= Interval)
		{
			Accumulator -= Interval;
			return true;
		}
		return false;
	}
};

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

