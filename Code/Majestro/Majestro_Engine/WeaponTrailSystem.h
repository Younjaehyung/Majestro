#pragma once
#include "System.h"
#include "WeaponTrailComponent.h"

class TransformSystem;

class WeaponTrailSystem : public System
{
public:
	WeaponTrailSystem(World* world);

	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;

private:
	void UpdateTrail(Entity entity, WeaponTrailComponent& trail, float deltaTime);
	void AddSample(Entity ownerEntity, WeaponTrailComponent& trail);
	void AgeSamples(WeaponTrailComponent& trail, float deltaTime);
};
