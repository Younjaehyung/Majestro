#pragma once
#include "System.h"
#include "NetTransformComponent.h"

class NetInterpolationSystem : public System
{
public:
	 NetInterpolationSystem(World* world);
	void Update(float dt) override;
private:
    
public:

private:

    double mServerHz = 60.0;
    double mFixedDtSec = 1.0 / 60.0;

};