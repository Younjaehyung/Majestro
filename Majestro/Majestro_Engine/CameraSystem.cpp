#include "pch.h"
#include "CameraSystem.h"
#include "Engine.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TagComponent.h"


CameraSystem::CameraSystem(World* world) : System(world)
{
}

void CameraSystem::Initialize()
{
}
	
void CameraSystem::Update(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };

	//TestUpdate(dt);
	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		
		std::vector<Entity> playerEntitys{ mWorld->GetEntitiesWithComponent<PlayerComponent>() };
		PlayerComponent* playerComponent = mWorld->GetComponent<PlayerComponent>(playerEntitys[0]);

		Vec3 pos = playerComponent->mTransformComponent.mLocalPosition;

		if (playerComponent->mPlayMode == "1PS") { //플레이어 시아로 변경 필요
			transformComponent->mLocalPosition = playerComponent->mTransformComponent.mLocalPosition;
			transformComponent->mLocalRotation = playerComponent->mTransformComponent.mLocalRotation;
			cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
		}
		else if (playerComponent->mPlayMode == "3PS") {
			pos.y += playerComponent->mHight;
			transformComponent->mLocalPosition = pos - playerComponent->mCameraLenth * playerComponent->mTransformComponent.GetLook();
			transformComponent->mLocalRotation = playerComponent->mTransformComponent.mLocalRotation;
			cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
		}
		
		else cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
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
