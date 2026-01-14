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
	void ConvertMove(NetEntityComponent*, SendRequest* );

private:
	SendRequest mSendReq;
};

