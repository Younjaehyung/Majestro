#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"

class UIRenderSystem : public System
{
public:
	UIRenderSystem(World* world);
	void Initialize();
	void Update();
};
