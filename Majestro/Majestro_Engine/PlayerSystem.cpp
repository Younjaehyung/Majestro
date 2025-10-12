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

	if (INPUT.GetKey(eKeyCode::A)) {
		transformComponent->mLocalPosition -= transformComponent->GetRight() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		transformComponent->mLocalPosition += transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		transformComponent->mLocalPosition -= transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		transformComponent->mLocalPosition += transformComponent->GetRight() * dt * 50.f;
	}

	//TestUpdate(dt);
	for (auto& entity : entitys) {
		//CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		//TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

		//transformComponent->FinalUpdate();
		//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
	}

}