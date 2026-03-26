#include "pch.h"
#include "CameraSystem.h"
#include "Engine.h"
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

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<CameraComponent>() };
	if (entitys.empty()) return;
	//TestUpdate(dt);

	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(entity);

		if (cameraTypeComponent) {

			auto& playerPosPool = mWorld->GetComponentPool<TransformComponent>();
			TransformComponent* playerPos = playerPosPool.GetComponent(cameraTypeComponent->mTargetID);

			auto& playerMovePool = mWorld->GetComponentPool<PlayerMovementComponent>();
			PlayerMovementComponent* movementComponent = playerMovePool.GetComponent(cameraTypeComponent->mTargetID);

			Vec3 pos = playerPos->mLocalPosition;

			if (cameraTypeComponent->mPlayMode == ONE_FPS) { //플레이어 시아로 변경 필요

				transformComponent->mLocalPosition = pos;
				transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
				transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
			}
			else if (cameraTypeComponent->mPlayMode == THREE_FPS) {

				transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
				transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
				transformComponent->FinalUpdate();
				

				Vec3 worldOffset = transformComponent->GetRight() * cameraTypeComponent->mOffset.x
					+ transformComponent->GetUp() * cameraTypeComponent->mOffset.y
					+ transformComponent->GetLook() * cameraTypeComponent->mOffset.z;

				Vec3 DestPos = pos + worldOffset - cameraTypeComponent->mCameraMaxLenth * transformComponent->GetLook();


				SweepHit best = mWorld->GetPhysicsWorld()->SphereSweepVsOBB(pos + cameraTypeComponent->mOffset, DestPos,
					cameraTypeComponent->mCameraSphereRadius);


				best.distance -= cameraTypeComponent->mCameraMargin;

				if (best.distance < cameraTypeComponent->mCameraMinLenth)
					best.distance = cameraTypeComponent->mCameraMinLenth;

				best.distance = cameraTypeComponent->mCameraMaxLenth;

				transformComponent->mLocalPosition = DestPos;// pos + cameraTypeComponent->mOffset - best.distance * transformComponent->GetLook();
			}
			else if (cameraTypeComponent->mPlayMode == THREE_RPG) {

				transformComponent->mLocalPosition = pos - cameraTypeComponent->mCameraMaxLenth * transformComponent->GetLook();
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
			// 카메라 쉐이크: 공격 등의 임팩트 시 pitch를 sin 파형으로 진동
		if (cameraTypeComponent && cameraTypeComponent->mShakeRemaining > 0.f)
		{
			cameraTypeComponent->mShakeRemaining -= dt;
			cameraTypeComponent->mShakeTimeAcc   += dt;

			float decay      = cameraTypeComponent->mShakeRemaining / cameraTypeComponent->mShakeDuration;
			float shakeAngle = sinf(cameraTypeComponent->mShakeTimeAcc * cameraTypeComponent->mShakeFrequency * XM_2PI)
			                   * cameraTypeComponent->mShakeMagnitude * decay;

			transformComponent->mLocalRotationE.x += shakeAngle;
		}

		transformComponent->FinalUpdate();
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
