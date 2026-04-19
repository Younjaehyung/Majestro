#pragma once
#include "World.h"
#include "System.h"

class PlayerMovementComponent;
class TransformComponent;


class MovementSystem :public System
{
public:
	MovementSystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

public: // 총알
	void RegisterActiveBullet(Entity bulletEntity);
	void UnregisterActiveBullet(Entity bulletEntity);

public: // 애니메이션
	void ResetTurnInPlace(PlayerMovementComponent* movementComponent);
	void UpdateBodyYawForAim(PlayerMovementComponent* movementComponent, TransformComponent* transformComponent, float dt, bool followCameraImmediately);

private:
	std::vector<EntityID> mActiveBulletEntityIds;
};

