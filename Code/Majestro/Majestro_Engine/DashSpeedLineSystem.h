#pragma once
#include "System.h"
#include "DashSpeedLineComponent.h"

class DashSpeedLineSystem : public System
{
public:
	DashSpeedLineSystem(World* world);

	std::vector<std::type_index> After() const override;
	void Update(float deltaTime) override;

private:
	void UpdateTrail(Entity entity, DashSpeedLineComponent& trail, float deltaTime);
	void AgeSamples(DashSpeedLineComponent& trail, float deltaTime);
	void AddSample(Entity ownerEntity, DashSpeedLineComponent& trail);
};
