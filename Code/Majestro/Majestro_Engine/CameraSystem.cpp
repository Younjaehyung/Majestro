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
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };

	//TestUpdate(dt);
	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(entity);

		auto& playerPosPool = mWorld->GetComponentPool<TransformComponent>();
		TransformComponent* playerPos = playerPosPool.GetComponent(cameraTypeComponent->mTargetID);

		auto& playerMovePool = mWorld->GetComponentPool<MovementComponent>();
		MovementComponent* movementComponent = playerMovePool.GetComponent(cameraTypeComponent->mTargetID);
		
		Vec3 pos = playerPos->mLocalPosition;

		if (cameraTypeComponent->mPlayMode == ONE_FPS) { //플레이어 시아로 변경 필요
			transformComponent->mLocalPosition = playerPos->mLocalPosition;
			transformComponent->mLocalRotation.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;
			//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
		}
		else if (cameraTypeComponent->mPlayMode == THREE_FPS) {
			pos.y += cameraTypeComponent->mCameraHight;
			transformComponent->mLocalPosition = pos - cameraTypeComponent->mCameraLenth * transformComponent->GetLook();
			transformComponent->mLocalRotation.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;
			
		}
		else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
			pos.y += cameraTypeComponent->mCameraHight;
			transformComponent->mLocalPosition = pos - cameraTypeComponent->mCameraLenth * transformComponent->GetLook();
			transformComponent->mLocalRotation.x = movementComponent->mCameraRotationX;
			transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;

		}

		cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
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
