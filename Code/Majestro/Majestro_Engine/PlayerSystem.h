#pragma once
#include "World.h"
#include "System.h"

class PlayerMovementComponent;
class MainPlayerComponent;

class PlayerSystem : public System
{
public:
	PlayerSystem(World* world);

	void Initialize();
	void Update(float dt);
	std::vector<std::type_index> After() const override;

private:
	float speed = 30.0f;
	const float DPI = 0.5f;
};
