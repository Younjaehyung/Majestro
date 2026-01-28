#include "pch.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"

TransformSystem::TransformSystem(World* world) : System(world)
{
	
}



void TransformSystem::Update(float dt) {
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<TransformComponent>() };
	for (auto& entity : entitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		if(transformComponent -> mIsStatic)
			continue;
		transformComponent->FinalUpdate();
	}

}