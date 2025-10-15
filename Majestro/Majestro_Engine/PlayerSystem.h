#pragma once
#include "World.h"
#include "System.h"

class PlayerComponent;

class PlayerSystem : public System
{
public:
	PlayerSystem(World* world);

	void Initialize();
	void Update(float dt);
	void Input(float dt, PlayerComponent* playerComponent);

private:
	float speed = 30.0f;
	const float DPI = 0.5f;
};
