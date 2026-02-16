#pragma once
#include "System.h"
#include "ServerCore.h"

class NetEntityComponent;

class NetSendSystem : public System
{
public:
	NetSendSystem(World* world);
	void Update(float dt) override;

private:
	void ConvertMove(NetEntityComponent*, SendRequest* , float);
	void ConvertState();
	void SendCollision();
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
};

