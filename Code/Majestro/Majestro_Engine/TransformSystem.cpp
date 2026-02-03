#include "pch.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"

TransformSystem::TransformSystem(World* world) : System(world)
{
	
}



void TransformSystem::Update(float dt) {
	auto view = mWorld->View<TransformComponent>();
	for (Entity entity : view) {

		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		
		if(transformComponent -> mIsStatic)
			continue;
		transformComponent->FinalUpdate();
	}

}