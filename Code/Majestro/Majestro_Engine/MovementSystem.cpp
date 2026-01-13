#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"

MovementSystem::MovementSystem(World* world) : System(world)
{

}



void MovementSystem::Update(float dt) {

	//movement
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };
	for (auto& entity : entitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

	}


	//terrain
	auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);

	std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityEntitys) {
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
	}

}