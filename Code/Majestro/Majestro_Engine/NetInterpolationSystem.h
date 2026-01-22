#pragma once
#include "System.h"

class NetInterpolationSystem : public System
{
public:
	 NetInterpolationSystem(World* world);
	void Update(float dt) override;
};