#include "pch.h"
#include "PlayerSystem.h"
#include "Engine.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
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
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
	PlayerComponent* playerComponent = mWorld->GetComponent<PlayerComponent>(entitys[0]);

	Input(dt, playerComponent);

	if (playerComponent->mPlayMode == "MainCamera") {
		transformComponent->mLocalPosition = playerComponent->mTransformComponent.mLocalPosition;
		transformComponent->mLocalRotation = playerComponent->mTransformComponent.mLocalRotation;
	}
	else if (playerComponent->mPlayMode == "1PS" || playerComponent->mPlayMode == "3PS") { //1,3��Ī
		transformComponent->mLocalPosition.x = playerComponent->mTransformComponent.mLocalPosition.x;
		transformComponent->mLocalPosition.z = playerComponent->mTransformComponent.mLocalPosition.z;
		transformComponent->mLocalRotation.y = playerComponent->mTransformComponent.mLocalRotation.y;
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