#pragma once
#include "System.h"

class SocketTrailSystem : public System
{
public:
	SocketTrailSystem(World* world);

	void Update(float deltaTime) override;
	std::vector<std::type_index> Before() const override;
	std::vector<std::type_index> After() const override;
};
