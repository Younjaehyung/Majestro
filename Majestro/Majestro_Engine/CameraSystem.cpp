#include "pch.h"
#include "CameraSystem.h"
#include "CameraComponent.h"
#include "TransformComponent.h"

CameraSystem::CameraSystem(World* world) : System(world)
{
}

void CameraSystem::Initialize()
{
}

void CameraSystem::Update()
{
	std::vector entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };

	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

		transformComponent->FinalUpdate();
		cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
	}

}
