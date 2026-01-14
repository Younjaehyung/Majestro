#pragma once
#include "System.h"
#include "NetIdMap.h"


class NetSendSystem : public System
{
public:
	NetSendSystem(World* world);
	void Update(double deltaTime);
public:

};

