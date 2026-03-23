#pragma once
#include "System.h"
#include "ServerCore.h"
#include <unordered_map>

class NetEntityComponent;

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


class NetSendSystem : public System
{
public:
	NetSendSystem(World* world);
	void Update(float dt) override;

private:
	void ConvertMove(NetEntityComponent*, SendRequest* , float);
	void SendAction();
	void SendCollision();
	void SendHealthEvents();
	void SendArmorEvents();
	void SendBulletDeactivateEvents();
	void SendEffectSpawnEvents();
	std::vector<uint32> CollectPlayerSessions() const;
private:
	SendRequest mSendReq;
	NetEntityComponent* mNetComp = nullptr;
private:
	static constexpr float mMoveSendHz = 30.0f;
	static constexpr int mMaxMoveBurst = 4;
	float mMoveSendAccumulator = 0.0f;
	float mMoveSendInterval = 1.0f / mMoveSendHz;

	uint32 mSequence = 0;

	RateLimiter mMovementRate{ 30.f };  // 이동 입력 30Hz
};

