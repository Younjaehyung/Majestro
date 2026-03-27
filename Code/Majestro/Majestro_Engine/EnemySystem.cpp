#include "pch.h"
#include "EnemySystem.h"
#include"TransformComponent.h"
#include "EnemyComponent.h"
#include "AnimationComponent.h"
#include "RenderComponent.h"
#include"MovementComponent.h"
#include"SimpleMath.h"


EnemySystem::EnemySystem(World* world) : System(world)
{

}



void EnemySystem::Update(float dt) {
	for (Entity entity : mWorld->View<EnemyComponent>()) {
		EnemyComponent* enemyComponent = mWorld->GetComponent<EnemyComponent>(entity);
		RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
		AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);
		if (enemyComponent == nullptr || renderComponent == nullptr || animationComponent == nullptr)
			continue;

		const bool isDead = (enemyComponent->mAnimStatePacket == static_cast<int>(EnemyAnimState::Dead));
		const uint32 entityId = entity.GetID();

		if (isDead) {
			enemyComponent->mDeadElapsedTime += dt;

			const uint32 deadClipIdx = static_cast<uint32>(EnemyAnimState::Dead);
			if (deadClipIdx < animationComponent->mAnimClips.size()) {
				const shared_ptr<Animator>& deadClip = animationComponent->mAnimClips.at(deadClipIdx);
				const float deadDuration = max(static_cast<float>(deadClip->mDuration), 0.f);
				renderComponent->mVisibility = enemyComponent->mDeadElapsedTime < deadDuration;
			}
			else {
				renderComponent->mVisibility = false;
			}
		}
		else {
			renderComponent->mVisibility = true;
			enemyComponent->mDeadElapsedTime = 0.f;
		}
	}


	//auto& transformPool = mWorld->GetComponentPool<TransformComponent>();


	//TransformComponent* playerPos = transformPool.GetComponent(cameraTypeComponent->mTargetID);


	
	/*for (Entity entity : mWorld->View<EnemyMovementComponent>()) {
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
	}*/

}