#pragma once
#include "System.h"
#include "NetIdMap.h"

class  EventManager;

class NetSendSystem : public System
{
public:
	NetSendSystem(World* world, EventManager* event);
	void Update(double deltaTime);
public:
	void ConvertInput(SendRequest* seq);
private:
	C2S_InputPacket mInputPacket{};
};

