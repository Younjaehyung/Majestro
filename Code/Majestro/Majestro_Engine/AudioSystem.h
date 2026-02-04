#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"

class AudioSystem : public System
{
public:
	AudioSystem(World* world);

	void Initialize();
	void Update(float);
	void Shutdown();
private:
	float time{};
};

