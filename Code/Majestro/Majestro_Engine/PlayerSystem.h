#pragma once
#include "World.h"
#include "System.h"

class ControllerComponent;
class MainPlayerComponent;

class PlayerSystem : public System
{
public:
	PlayerSystem(World* world);

	void Initialize();
	void Update(float dt);
	void Input(float dt, ControllerComponent* playerComponent , MainPlayerComponent* mainPlayerComponent, bool beatHit);

private:
	float speed = 30.0f;
	const float DPI = 0.5f;
};
