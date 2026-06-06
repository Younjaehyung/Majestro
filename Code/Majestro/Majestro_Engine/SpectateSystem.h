#pragma once
#include "World.h"
#include "System.h"

class CameraTypeComponent;
class DeathCamComponent;

class SpectateSystem : public System
{
public:
	SpectateSystem(World* world);

	void Update(float dt);

private:
	// 관전 대상 순환 선택
	EntityID PickSpectateTarget(EntityID self, EntityID current, bool forward);
};
