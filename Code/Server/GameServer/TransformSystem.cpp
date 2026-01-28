#include "pch.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "GravityComponent.h"

TransformSystem::TransformSystem(World* world) : System(world)
{
	
}



void TransformSystem::Update(float dt) {
	if (false == mWorld->HasComponentPool<TransformComponent>())return;
	auto view = mWorld->View<TransformComponent>();
	for (Entity entity : view) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		transformComponent->FinalUpdate();
	}

}