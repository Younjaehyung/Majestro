#pragma once
#include "System.h"

class NetRecvSystem : public System
{
public:
	NetRecvSystem(World* world);
	void Update(float dt) override;
};

