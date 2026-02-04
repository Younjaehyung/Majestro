#include "pch.h"
#include "CameraSystem.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TagComponent.h"
#include "MovementComponent.h"


CameraSystem::CameraSystem(World* world) : System(world)
{
}

void CameraSystem::Initialize()
{
}

void CameraSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<TransformComponent>())return;


	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };

	//TestUpdate(dt);
	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(entity);

		auto& playerPosPool = mWorld->GetComponentPool<TransformComponent>();
		TransformComponent* playerPos = playerPosPool.GetComponent(cameraTypeComponent->mTargetID);

		auto& playerMovePool = mWorld->GetComponentPool<PlayerMovementComponent>();
		PlayerMovementComponent* movementComponent = playerMovePool.GetComponent(cameraTypeComponent->mTargetID);

		Vec3 pos = playerPos->mLocalPosition;

		if (cameraTypeComponent->mPlayMode == ONE_FPS) { //플레이어 시아로 변경 필요
			pos.y += cameraTypeComponent->mCameraHight;
			transformComponent->mLocalPosition = pos;
			transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
		}
		else if (cameraTypeComponent->mPlayMode == THREE_FPS) {
			pos.y += cameraTypeComponent->mCameraHight;
			transformComponent->mLocalPosition = pos - cameraTypeComponent->mCameraLenth * transformComponent->GetLook();
			transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;

		}
		else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
			pos.y += cameraTypeComponent->mCameraHight;
			transformComponent->mLocalPosition = pos - cameraTypeComponent->mCameraLenth * transformComponent->GetLook();
			transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
		}
		else {
			Vec3 forward = transformComponent->GetLook();
			Vec3 right = transformComponent->GetRight();
			Vec3 up = { 0,1,0 };

			float ix = movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
			float iz = movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)
			float iy = movementComponent->mMovingDirection.y;  // W/S   (-1 ~ 1)

			Vec3 desired = forward * iz + right * ix + up * iy;

			// 정규화
			if (desired.LengthSquared() > 0.0001f)
				desired.Normalize();

			transformComponent->mLocalPosition += desired * dt * cameraTypeComponent->mCameraMoveSpeed;

			transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
		}

		cameraComponent->FinalUpdate(transformComponent->GetWorldMatrix().Invert());
	}

}


void CameraSystem::TestUpdate(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);


	/*if (INPUT.GetKey(eKeyCode::A)) {
		transformComponent->mLocalPosition -= transformComponent->GetRight() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		transformComponent->mLocalPosition += transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		transformComponent->mLocalPosition -= transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		transformComponent->mLocalPosition += transformComponent->GetRight()* dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::Q)) {
		transformComponent->mLocalPosition -= transformComponent->GetUp() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		transformComponent->mLocalPosition += transformComponent->GetUp() * dt * 50.f;
	}*/


	/*const float DPI = 0.5f;
	if (INPUT.GetMouseState().LeftDown) {
		transformComponent->mLocalRotation.x += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
		transformComponent->mLocalRotation.y += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
		INPUT.MouseStateClear();
	}*/



}
