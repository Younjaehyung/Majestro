#pragma once
#include "System.h"


class SocketSystem : public System
{
public:
	SocketSystem(World* world);

	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;
};
