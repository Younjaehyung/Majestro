#include "pch.h"
#include "CameraSystem.h"
#include "Engine.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "InputManager.h"

CameraSystem::CameraSystem(World* world) : System(world)
{
}

void CameraSystem::Initialize()
{
}
	
void CameraSystem::Update()
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };
	TestUpdate();
	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

		transformComponent->FinalUpdate();
		cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
	}
	
}

void CameraSystem::TestUpdate()
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
	

	if (INPUT.GetKey(eKeyCode::A)) {
		transformComponent->mLocalPosition.x--;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		transformComponent->mLocalPosition.z++;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		transformComponent->mLocalPosition.z--;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		transformComponent->mLocalPosition.x++;
	}
}
