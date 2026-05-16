#include "pch.h"
#include "EnemySystem.h"
#include"TransformComponent.h"
#include "EnemyComponent.h"
#include "AnimationComponent.h"
#include "RenderComponent.h"
#include"MovementComponent.h"
#include"SimpleMath.h"
#include "RenderSystem.h"

namespace
{
	constexpr uint8 kPianomanType = 1;

	constexpr float kEnemyAggroRange = 1000.0f;
	constexpr float kPianomanMeleeRange = 160.0f;
	constexpr float kCircleHeightOffset = 5.0f;
	constexpr int kCircleSegments = 40;

	void SubmitDebugCircle(const Vec3& center, float radius, const Vec4& color, int segments = kCircleSegments)
	{
		if (radius <= 0.0f || segments < 3)
			return;

		const float step = DirectX::XM_2PI / static_cast<float>(segments);
		for (int i = 0; i < segments; ++i)
		{
			const float a0 = step * static_cast<float>(i);
			const float a1 = step * static_cast<float>(i + 1);

			const Vec3 p0{
				center.x + std::cos(a0) * radius,
				center.y,
				center.z + std::sin(a0) * radius
			};
			const Vec3 p1{
				center.x + std::cos(a1) * radius,
				center.y,
				center.z + std::sin(a1) * radius
			};

			RenderSystem::SubmitDebugLine(p0, p1, color);
		}
	}

	void SubmitEnemyRangeCircles(const TransformComponent* transformComponent, const EnemyComponent* enemyComponent)
	{
		if (transformComponent == nullptr || enemyComponent == nullptr)
			return;

		Vec3 center = transformComponent->mLocalPosition;
		center.y += kCircleHeightOffset;

		SubmitDebugCircle(center, kEnemyAggroRange, Vec4(0.f, 1.f, 0.f, 1.f));

		if (enemyComponent->mEnemyType == kPianomanType)
			SubmitDebugCircle(center, kPianomanMeleeRange, Vec4(1.f, 0.f, 0.f, 1.f), 24);
	}
}


EnemySystem::EnemySystem(World* world) : System(world)
{

}



void EnemySystem::Update(float dt) {
	for (Entity entity : mWorld->View<EnemyComponent>()) {
		EnemyComponent* enemyComponent = mWorld->GetComponent<EnemyComponent>(entity);
		RenderComponent* renderComponent = mWorld->GetComponent<RenderComponent>(entity);
		AnimationComponent* animationComponent = mWorld->GetComponent<AnimationComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		if (enemyComponent == nullptr || renderComponent == nullptr || animationComponent == nullptr)
			continue;

		const bool isDead = (enemyComponent->mAnimStatePacket == static_cast<int>(EnemyAnimState::Dead));
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

		if (!isDead && RenderSystem::GetDrawEnemyRanges())
			SubmitEnemyRangeCircles(transformComponent, enemyComponent);
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
