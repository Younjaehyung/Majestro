#include "pch.h"
#include "EnemySystem.h"
#include"TransformComponent.h"
#include "EnemyComponent.h"
#include "AnimationComponent.h"
#include "BulletComponent.h"
#include "RenderComponent.h"
#include"MovementComponent.h"
#include"SimpleMath.h"
#include "RenderSystem.h"

namespace
{
	constexpr uint8 kPianomanType = 1;
	constexpr uint8 kBongomanType = 2;
	constexpr uint8 kBrassType = 5;

	constexpr float kEnemyAggroRange = 1000.0f;
	constexpr float kBongomanAggroRange = 300.0f;
	constexpr float kBrassWakeRange = 3000.0f;
	constexpr float kPianomanMeleeRange = 160.0f;
	constexpr float kPianomanAttackRadius = 200.0f;
	constexpr float kBongomanAttackRadius = 500.0f;
	constexpr float kPlayerMeleeAttackRadius = 500.0f;
	constexpr float kGuitarAttack2ExplosionRadius = 300.0f;
	constexpr float kPlayerMeleeAttackAngleDegrees = 150.0f;
	constexpr float kMeleeAttackForwardDistance = 3.0f;
	constexpr float kAttackDebugDuration = 1.0f;
	constexpr float kAttackDebugHeight = 220.0f;
	constexpr int kAttackDebugCylinderRings = 5;
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

	void SubmitDebugCircleXZ(const Vec3& center, float radius, const Vec4& color, int segments = kCircleSegments)
	{
		SubmitDebugCircle(center, radius, color, segments);
	}

	void SubmitDebugCircleXY(const Vec3& center, float radius, const Vec4& color, int segments = kCircleSegments)
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
				center.y + std::sin(a0) * radius,
				center.z
			};
			const Vec3 p1{
				center.x + std::cos(a1) * radius,
				center.y + std::sin(a1) * radius,
				center.z
			};

			RenderSystem::SubmitDebugLine(p0, p1, color);
		}
	}

	void SubmitDebugCircleYZ(const Vec3& center, float radius, const Vec4& color, int segments = kCircleSegments)
	{
		if (radius <= 0.0f || segments < 3)
			return;

		const float step = DirectX::XM_2PI / static_cast<float>(segments);
		for (int i = 0; i < segments; ++i)
		{
			const float a0 = step * static_cast<float>(i);
			const float a1 = step * static_cast<float>(i + 1);

			const Vec3 p0{
				center.x,
				center.y + std::cos(a0) * radius,
				center.z + std::sin(a0) * radius
			};
			const Vec3 p1{
				center.x,
				center.y + std::cos(a1) * radius,
				center.z + std::sin(a1) * radius
			};

			RenderSystem::SubmitDebugLine(p0, p1, color);
		}
	}

	void SubmitDebugSphere(const Vec3& center, float radius, const Vec4& color)
	{
		SubmitDebugCircleXZ(center, radius, color, 28);
		SubmitDebugCircleXY(center, radius, color, 28);
		SubmitDebugCircleYZ(center, radius, color, 28);
	}

	bool IsEnemyRangedSkill(SkillType skillType)
	{
		return skillType == SkillType::HornAttack ||
			skillType == SkillType::BrassSkill2 ||
			skillType == SkillType::BrassSkill4;
	}

	bool IsPlayerRangedSkill(SkillType skillType)
	{
		return skillType == SkillType::BaseAttack ||
			skillType == SkillType::BaseAttack2 ||
			skillType == SkillType::BaseAttack3 ||
			skillType == SkillType::BaseSkill1 ||
			skillType == SkillType::GuitarAttack_1 ||
			skillType == SkillType::GuitarAttack_2 ||
			skillType == SkillType::GuitarAttack_3;
	}

	void SubmitEnemyRangeCircles(World* world, const TransformComponent* transformComponent, const EnemyComponent* enemyComponent)
	{
		if (world == nullptr || transformComponent == nullptr || enemyComponent == nullptr)
			return;

		Vec3 center = transformComponent->mLocalPosition;
		center.y += kCircleHeightOffset;

		if (enemyComponent->mEnemyType == kBrassType)
		{
			SubmitDebugCircle(center, kBrassWakeRange, Vec4(0.f, 1.f, 0.f, 1.f), 48);
		}
		else
		{
			const float aggroRange =
				enemyComponent->mEnemyType == kBongomanType ? kBongomanAggroRange : kEnemyAggroRange;
			SubmitDebugCircle(center, aggroRange, Vec4(0.f, 1.f, 0.f, 1.f));
		}

		if (enemyComponent->mEnemyType == kPianomanType)
			SubmitDebugCircle(center, kPianomanMeleeRange, Vec4(1.f, 0.f, 0.f, 1.f), 24);
	}

	void SubmitDebugCylinder(const Vec3& center, float radius, float height, const Vec4& color, int ringCount = kAttackDebugCylinderRings)
	{
		if (ringCount < 2 || radius <= 0.0f || height <= 0.0f)
			return;

		const float minY = center.y;
		const float stepY = height / static_cast<float>(ringCount - 1);
		for (int i = 0; i < ringCount; ++i)
		{
			Vec3 ringCenter = center;
			ringCenter.y = minY + stepY * static_cast<float>(i);
			SubmitDebugCircle(ringCenter, radius, color, 28);
		}

		constexpr int kVerticalSegments = 8;
		for (int i = 0; i < kVerticalSegments; ++i)
		{
			const float angle = DirectX::XM_2PI * (static_cast<float>(i) / static_cast<float>(kVerticalSegments));
			Vec3 bottom(
				center.x + std::cos(angle) * radius,
				minY,
				center.z + std::sin(angle) * radius);
			Vec3 top = bottom;
			top.y += height;
			RenderSystem::SubmitDebugLine(bottom, top, color);
		}
	}

	void SubmitDebugSectorCylinder(
		const Vec3& center,
		const Vec3& forward,
		float radius,
		float angleDegrees,
		float height,
		const Vec4& color,
		int ringCount = kAttackDebugCylinderRings)
	{
		if (ringCount < 2 || radius <= 0.0f || height <= 0.0f || angleDegrees <= 0.0f)
			return;

		Vec3 flatForward = forward;
		flatForward.y = 0.0f;
		if (flatForward.LengthSquared() <= 1e-6f)
			flatForward = Vec3::Forward;
		else
			flatForward.Normalize();

		const float baseYaw = std::atan2(flatForward.z, flatForward.x);
		const float halfAngleRad = DirectX::XMConvertToRadians(angleDegrees * 0.5f);
		const int arcSegments = 20;
		const float minY = center.y;
		const float stepY = height / static_cast<float>(ringCount - 1);

		for (int ring = 0; ring < ringCount; ++ring)
		{
			const float y = minY + stepY * static_cast<float>(ring);
			Vec3 ringCenter = center;
			ringCenter.y = y;

			Vec3 prevPoint = ringCenter;
			bool hasPrevPoint = false;
			for (int i = 0; i <= arcSegments; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(arcSegments);
				const float yaw = baseYaw - halfAngleRad + (halfAngleRad * 2.0f) * t;
				const Vec3 point(
					ringCenter.x + std::cos(yaw) * radius,
					y,
					ringCenter.z + std::sin(yaw) * radius);

				RenderSystem::SubmitDebugLine(ringCenter, point, color);
				if (hasPrevPoint)
					RenderSystem::SubmitDebugLine(prevPoint, point, color);

				prevPoint = point;
				hasPrevPoint = true;
			}
		}

		const int verticalSegments = 6;
		for (int i = 0; i <= verticalSegments; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(verticalSegments);
			const float yaw = baseYaw - halfAngleRad + (halfAngleRad * 2.0f) * t;
			Vec3 bottom(
				center.x + std::cos(yaw) * radius,
				minY,
				center.z + std::sin(yaw) * radius);
			Vec3 top = bottom;
			top.y += height;
			RenderSystem::SubmitDebugLine(bottom, top, color);
		}
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

		const bool isDead = (enemyComponent->mAnimState == static_cast<int>(EnemyAnimState::Dead));
		if (isDead) {
			if (enemyComponent->mEnemyType == kBrassType &&
				enemyComponent->mDeadElapsedTime <= 0.0f &&
				transformComponent != nullptr &&
				mWorld->GetEventManager() != nullptr)
			{
				mWorld->GetEventManager()->Enqueue(EvVfxSpawnRequest{
					SkillType::BrassSkill1,
					static_cast<uint8>(EffectSpawnReason::Death),
					transformComponent->mLocalPosition,
					transformComponent->mLocalRotationE,
					0 });
			}

			enemyComponent->mDeadElapsedTime += dt;

			const uint32 deadClipIdx = static_cast<uint32>(EnemyAnimState::Dead);
			if (deadClipIdx < animationComponent->mAnimClips.size()) {
				const shared_ptr<Animator>& deadClip = animationComponent->mAnimClips.at(deadClipIdx);
				const float deadDuration = max(static_cast<float>(deadClip->mDuration), 0.f);

				// 사망 애니 후반부(60% 지점)부터 디졸브 시작해서 애니 종료 시점에 완전 소멸
				constexpr float kDissolveStartRatio = 0.6f;
				const float dissolveStart = deadDuration * kDissolveStartRatio;
				const float dissolveSpan = max(deadDuration - dissolveStart, 1e-4f);
				const float dissolveT = (enemyComponent->mDeadElapsedTime - dissolveStart) / dissolveSpan;
				renderComponent->mDissolve = std::clamp(dissolveT, 0.f, 1.f);

				renderComponent->mVisibility = enemyComponent->mDeadElapsedTime < deadDuration;
			}
			else {
				renderComponent->mVisibility = false;
			}
		}
		else {
			renderComponent->mVisibility = true;
			enemyComponent->mDeadElapsedTime = 0.f;
			renderComponent->mDissolve = 0.f;
		}

			if (!isDead && RenderSystem::GetDrawEnemyRanges())
				SubmitEnemyRangeCircles(mWorld, transformComponent, enemyComponent);
		}

	if (RenderSystem::GetDrawEnemyAttackRanges() || RenderSystem::GetDrawPlayerAttackRanges())
	{
		const Vec4 color = Vec4(1.0f, 1.0f, 0.f, 1.0f);
		for (Entity bulletEntity : mWorld->View<BulletComponent>())
		{
			BulletComponent* bulletComponent = mWorld->GetComponent<BulletComponent>(bulletEntity);
			TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
			if (!bulletComponent || !bulletTransform || !bulletComponent->mIsActive)
				continue;

			const SkillType skillType = bulletComponent->mType;
			const bool drawEnemyRanged = RenderSystem::GetDrawEnemyAttackRanges() && IsEnemyRangedSkill(skillType);
			const bool drawPlayerRanged = RenderSystem::GetDrawPlayerAttackRanges() && IsPlayerRangedSkill(skillType);
			if (!drawEnemyRanged && !drawPlayerRanged)
				continue;

			const float radius = (std::max)(0.0f, bulletTransform->mLocalScale.x);
			if (radius <= 0.0f)
				continue;

			SubmitDebugSphere(bulletTransform->mLocalPosition, radius, color);
		}

		UpdateAttackDebugIndicators(dt);
	}

	if (mWorld->GetEventManager())
	{
		mWorld->GetEventManager()->Consume<EvEnemyAttackDebug>([&](const EvEnemyAttackDebug& e)
		{
			Vec3 forward = Vec3::Forward;
			const float yawRad = DirectX::XMConvertToRadians(e.rotation.y);
			forward.x = std::sin(yawRad);
			forward.z = std::cos(yawRad);
			forward.y = 0.0f;
			if (forward.LengthSquared() > 1e-8f)
				forward.Normalize();
			else
				forward = Vec3::Forward;

			float radius = 0.0f;
			const Vec4 color = Vec4(1.0f, 1.0f, 0.f, 1.0f);
			bool isPlayerAttack = false;
			if (e.skillType == SkillType::PianoAttack)
			{
				if (!RenderSystem::GetDrawEnemyAttackRanges())
					return;
				radius = kPianomanAttackRadius;
			}
			else if (e.skillType == SkillType::SlimeAttack)
			{
				if (!RenderSystem::GetDrawEnemyAttackRanges())
					return;
				radius = kBongomanAttackRadius;
			}
			else if (e.skillType == SkillType::FlyAttack)
			{
				if (!RenderSystem::GetDrawEnemyAttackRanges())
					return;
				radius = kBongomanAttackRadius;
			}
				else if (e.skillType == SkillType::BongoAttack)
				{
					if (!RenderSystem::GetDrawEnemyAttackRanges())
						return;
					radius = kBongomanAttackRadius;
				}
				else if (e.skillType == SkillType::DragonSkill1)
				{
					if (!RenderSystem::GetDrawEnemyAttackRanges())
						return;
					radius = 320.0f;
				}
				else if (e.skillType == SkillType::DragonSkill3)
				{
					if (!RenderSystem::GetDrawEnemyAttackRanges())
						return;
					radius = 420.0f;
				}
				else if (e.skillType == SkillType::DragonSkill4)
				{
					if (!RenderSystem::GetDrawEnemyAttackRanges())
						return;
					radius = 550.0f;
				}
				else if (e.skillType == SkillType::DrumAttack ||
					e.skillType == SkillType::DrumAttack3 ||
					e.skillType == SkillType::DrumSkill1 ||
					e.skillType == SkillType::GuitarAttack ||
					e.skillType == SkillType::GuitarSkill1)
			{
				if (!RenderSystem::GetDrawPlayerAttackRanges())
					return;
				isPlayerAttack = true;
				radius = kPlayerMeleeAttackRadius;
			}
			else if (e.skillType == SkillType::GuitarAttack_2 &&
				e.reason == static_cast<uint8>(EffectSpawnReason::CollisionEntity))
			{
				if (!RenderSystem::GetDrawPlayerAttackRanges())
					return;
				isPlayerAttack = true;
				radius = kGuitarAttack2ExplosionRadius;
			}

			if (radius <= 0.0f)
				return;

			AttackDebugIndicator indicator;
			indicator.center = e.positionIsCenter ? e.position : (e.position + forward * kMeleeAttackForwardDistance);
			indicator.center.y += kCircleHeightOffset;
			indicator.forward = forward;
			indicator.radius = radius;
			indicator.angleDegrees =
				(e.skillType == SkillType::DrumSkill1) ? 360.0f :
				(isPlayerAttack ? kPlayerMeleeAttackAngleDegrees : 360.0f);
			indicator.remainingTime = kAttackDebugDuration;
			indicator.color = color;
			indicator.isPlayerAttack = isPlayerAttack;
			indicator.isSector = isPlayerAttack && e.skillType != SkillType::DrumSkill1;
			indicator.isSphere = (e.skillType == SkillType::GuitarAttack_2);
			mAttackDebugIndicators.push_back(indicator);
		});
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

void EnemySystem::UpdateAttackDebugIndicators(float dt)
{
	for (AttackDebugIndicator& indicator : mAttackDebugIndicators)
	{
		if (indicator.remainingTime <= 0.0f)
			continue;

		indicator.remainingTime -= dt;
		if (indicator.remainingTime <= 0.0f)
			continue;

		if (indicator.isPlayerAttack)
		{
			if (!RenderSystem::GetDrawPlayerAttackRanges())
				continue;
		}
		else
		{
			if (!RenderSystem::GetDrawEnemyAttackRanges())
				continue;
		}

		if (indicator.isSphere)
			SubmitDebugSphere(indicator.center, indicator.radius, indicator.color);
		else if (indicator.isSector)
			SubmitDebugSectorCylinder(
				indicator.center,
				indicator.forward,
				indicator.radius,
				indicator.angleDegrees,
				kAttackDebugHeight,
				indicator.color);
		else
			SubmitDebugCylinder(indicator.center, indicator.radius, kAttackDebugHeight, indicator.color);
	}

	mAttackDebugIndicators.erase(
		std::remove_if(
			mAttackDebugIndicators.begin(),
			mAttackDebugIndicators.end(),
			[](const AttackDebugIndicator& indicator)
			{
				return indicator.remainingTime <= 0.0f;
			}),
		mAttackDebugIndicators.end());
}
