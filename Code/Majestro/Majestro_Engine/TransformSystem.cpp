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
		transformComponent->FinalUpdate();
	}


	//auto& terrains = mWorld->GetComponentPool<TerrainComponent>();
	auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);
	

	std::vector<Entity> gravityentitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityentitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		float terrainHeight = terrainComponent->GetHeightAtWorldPosition(transformComponent->mLocalPosition);

		if (gravityComponent->mHight <= terrainHeight) {
			gravityComponent->mHight = terrainHeight;
			gravityComponent->mGravity = 0.0f;
		}
		else {
			gravityComponent->mGravity += gravityComponent->mGravityA * dt;
			gravityComponent->mHight -= gravityComponent->mGravity;
		}

		transformComponent->mLocalPosition.y = gravityComponent->mHight;
		//transformComponent->FinalUpdate();
	}

}