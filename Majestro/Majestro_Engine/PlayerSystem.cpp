#include "pch.h"
#include "PlayerSystem.h"
#include "Engine.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<PlayerComponent, TransformComponent>() };

	if (INPUT.GetKeyDown(eKeyCode::F1)) {
		//printf("f4");
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<PlayerComponent>(entitys[0]);
		mWorld->AddComponent<PlayerComponent>(mainPlayerEntitys[0], *t, ONE_FPS);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F2)) {
		//printf("f4");
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<PlayerComponent>(entitys[0]);
		mWorld->AddComponent<PlayerComponent>(mainPlayerEntitys[0], *t, THREE_FPS);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F3)) {
		//printf("f4");
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<PlayerComponent>(entitys[0]);
		mWorld->AddComponent<PlayerComponent>(mainPlayerEntitys[0], *t, THREE_RPG);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F4)) {
		//printf("f4");
		std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainCameraEntitys[0]);
		mWorld->RemoveComponent<PlayerComponent>(entitys[0]);
		mWorld->AddComponent<PlayerComponent>(mainCameraEntitys[0], *t, MAIN_CAMERA);
	}
	else {

		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
		PlayerComponent* playerComponent = mWorld->GetComponent<PlayerComponent>(entitys[0]);


		Input(dt, playerComponent);

		if (playerComponent->mPlayMode == MAIN_CAMERA) {
			transformComponent->mLocalPosition = playerComponent->mTransformComponent.mLocalPosition;
			transformComponent->mLocalRotation = playerComponent->mTransformComponent.mLocalRotation;
		}
		else if (playerComponent->mPlayMode == ONE_FPS || playerComponent->mPlayMode == THREE_FPS) {
			transformComponent->mLocalPosition.x = playerComponent->mTransformComponent.mLocalPosition.x;
			transformComponent->mLocalPosition.z = playerComponent->mTransformComponent.mLocalPosition.z;
			transformComponent->mLocalRotation.y = playerComponent->mTransformComponent.mLocalRotation.y;
		}
		else if (playerComponent->mPlayMode == THREE_RPG) {
			transformComponent->mLocalPosition.x = playerComponent->mTransformComponent.mLocalPosition.x;
			transformComponent->mLocalPosition.z = playerComponent->mTransformComponent.mLocalPosition.z;
			//if(move)
			//transformComponent->mLocalRotation.y = playerComponent->mTransformComponent.mLocalRotation.y;
		}

		transformComponent->FinalUpdate();

		playerComponent->mTransformComponent.mLocalPosition = transformComponent->mLocalPosition;
		playerComponent->mTransformComponent.FinalUpdate();

		//TestUpdate(dt);
		for (auto& entity : entitys) {
			//CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
			//TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

			//transformComponent->FinalUpdate();
			//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
		}
	}
}

void PlayerSystem::Input(float dt, PlayerComponent* playerComponent)
{


	if (INPUT.GetKey(eKeyCode::A)) {
		playerComponent->mTransformComponent.mLocalPosition -= playerComponent->mTransformComponent.GetRight() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		playerComponent->mTransformComponent.mLocalPosition += playerComponent->mTransformComponent.GetLook() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		playerComponent->mTransformComponent.mLocalPosition -= playerComponent->mTransformComponent.GetLook() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		playerComponent->mTransformComponent.mLocalPosition += playerComponent->mTransformComponent.GetRight() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::Q)) {
		playerComponent->mTransformComponent.mLocalPosition -= playerComponent->mTransformComponent.GetUp() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		playerComponent->mTransformComponent.mLocalPosition += playerComponent->mTransformComponent.GetUp() * dt * speed;
	}
	if (INPUT.GetMouseState().LeftDown) {
		playerComponent->mTransformComponent.mLocalRotation.x += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
		playerComponent->mTransformComponent.mLocalRotation.y += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
		INPUT.MouseStateClear();
	}
}