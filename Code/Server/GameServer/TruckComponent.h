#pragma once
#include "Component.h"

class TruckComponent : public Component<TruckComponent>
{
public:
	float mSpeed = 0.f;
	float mHealth = 100.f;
	bool mIsDestroyed = false;
	

};

