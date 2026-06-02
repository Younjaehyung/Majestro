#pragma once
#include "System.h"
#include "WeaponTrailComponent.h"

class TransformSystem;
class AnimationComponent;

class WeaponTrailSystem : public System
{
public:
	WeaponTrailSystem(World* world);

	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;

private:
	bool IsAnimationWindowActive(Entity sourceEntity, const WeaponTrailComponent& trail) const;
	void UpdateTrail(Entity entity, WeaponTrailComponent& trail, float deltaTime);
	void AddSample(Entity ownerEntity, WeaponTrailComponent& trail);
	void AgeSamples(WeaponTrailComponent& trail, float deltaTime);
};
