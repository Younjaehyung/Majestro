#include "pch.h"
#include "CollisionSystem.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "MovementComponent.h"


CollisionSystem::CollisionSystem(World* world) : System(world)
{

}



void CollisionSystem::Update(float dt) {

	auto& transformPool = mWorld->GetComponentPool<TransformComponent>();


	//TransformComponent* playerPos = transformPool.GetComponent(cameraTypeComponent->mTargetID);


	//std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<BoxColliderComponent>() };
	std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };


	for (auto& entity : enemyEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		//EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		BoxColliderComponent* boxColliderComponent = mWorld->GetComponent<BoxColliderComponent>(entity);


		mBoundingBoxA.Center = boxColliderComponent->Center;
		mBoundingBoxA.Extents = boxColliderComponent->HalfExtents;
		mBoundingBoxA.Orientation = Vec4(transformComponent->mLocalRotation) ;



	}

}