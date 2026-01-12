#pragma once
#include "System.h"
#include "NetIdMap.h"
class INetSendSink;
class MainNetSendSink;

class NetSendSystem : public System
{
public:
	NetSendSystem(World* world, shared_ptr<NetIdMap>& );
	virtual ~NetSendSystem();
	void Update(double deltaTime);
public:
	shared_ptr<NetIdMap> mNetSendSink;
};

