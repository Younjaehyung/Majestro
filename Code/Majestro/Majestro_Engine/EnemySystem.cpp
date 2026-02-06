#include "pch.h"
#include "EnemySystem.h"
#include"TransformComponent.h"
#include"MovementComponent.h"
#include"SimpleMath.h"


EnemySystem::EnemySystem(World* world) : System(world)
{

}



void EnemySystem::Update(float dt) {

	auto& transformPool = mWorld->GetComponentPool<TransformComponent>();


	//TransformComponent* playerPos = transformPool.GetComponent(cameraTypeComponent->mTargetID);


	
	for (Entity entity : mWorld->View<EnemyMovementComponent>()) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		Vec3 dir;
		float maxLen = std::numeric_limits<float>::infinity();

		if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
		std::vector<Entity> playerEntitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };
		
		for (auto& entity : playerEntitys) {
			TransformComponent* playerPos = transformPool.GetComponent(entity.GetID());

			float len = Vec3::DistanceSquared(transformComponent->mLocalPosition, playerPos->mLocalPosition);
			maxLen = min(maxLen, len);
			if(len ==maxLen) dir = playerPos->mLocalPosition - transformComponent->mLocalPosition;
		}
		dir.y = 0;
		dir.Normalize();
		enemyMovementComponent->mMovingDirection = dir;
	}

}