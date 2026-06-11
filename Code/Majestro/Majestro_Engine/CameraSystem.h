#pragma once
#include "World.h"
#include "System.h"

class CameraComponent;
class CameraTypeComponent;
class TransformComponent;
class PlayerMovementComponent;
class DeathCamComponent;

class CameraSystem : public System
{
public:
	CameraSystem(World* world);

	void Initialize();
	void Update(float dt);
	void TestUpdate(float dt);

private:
	// THREE_FPS / THREE_RPG 공용 스프링암 궤도 카메라
	void UpdateOrbitCamera(CameraTypeComponent* camType, TransformComponent* transform,
		const Vec3& playerPos, PlayerMovementComponent* movement, float dt);

	// MAIN_CAMERA 자유 비행 카메라
	void UpdateFreeCamera(CameraTypeComponent* camType, TransformComponent* transform, float dt);

	// 사망 카메라
	void UpdateDeathCamera(CameraTypeComponent* camType, DeathCamComponent* death,
		TransformComponent* transform, TransformComponent* selfPlayer, float dt);

	void CameraShake(CameraTypeComponent* camType, TransformComponent* transform, float dt);
};

