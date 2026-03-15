#pragma once
#include "System.h"
#include "BulletComponent.h"

class BulletFireEventSystem : public System
{
public:
	BulletFireEventSystem(World* world);
	void Update(float dt) override;

private:
	void ActivateBulletAndNotify(Entity playerEntity, SkillType bulletType);
	std::vector<uint32> CollectPlayerSessions() const;
};