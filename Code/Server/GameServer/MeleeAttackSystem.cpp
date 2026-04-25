#include "pch.h"
#include "MeleeAttackSystem.h"

#include "World.h"
#include "EventManager.h"
#include "TransformComponent.h"
#include "InputComponent.h"
#include "HealthComponent.h"
#include "BulletComponent.h"
#include "BuffComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"

namespace
{
	constexpr float kDegToRad = 0.01745329251994329577f;

	Vec3 GetCameraForwardFromInput(const InputComponent& input)
	{
		const float yawRad = input.Yaw * kDegToRad;
		const float pitchRad = -input.Pitch * kDegToRad;

		const float cosPitch = std::cos(pitchRad);
		Vec3 forward;
		forward.x = std::sin(yawRad) * cosPitch;
		forward.y = std::sin(pitchRad);
		forward.z = std::cos(yawRad) * cosPitch;

		if (forward.LengthSquared() <= 0.0001f)
			return Vec3::Forward;

		forward.Normalize();
		return forward;
	}

	struct MeleeAttackStat
	{
		float damage = 10.0f;
		float forwardDistance = 3.0f;
		float radius = 2.0f;
		float knockbackDistance = 0.0f;
	};

	MeleeAttackStat GetMeleeAttackStat(SkillType type)
	{
		switch (type)
		{
		case SkillType::DrumAttack:
			return { 25.0f, 3.0f, 100.0f , 0.0f};
		case SkillType::DrumSkill1:
			return { 75.0f, 3.0f, 100.0f , 0.0f };
		case SkillType::GuitarAttack:
			return { 25.0f, 3.0f, 100.2f, 0.0f };
		case SkillType::GuitarSkill1:
			return { 30.0f, 3.0f, 100.2f, 0.0f };
		case SkillType::PianoAttack:
			return { 20.0f, 3.0f, 100.2f, 0.0f };
		case SkillType::BongoAttack:
			return { 40.0f, 3.0f, 100.2f, 0.0f };
		default:
			return {};
		}
	}
}

MeleeAttackSystem::MeleeAttackSystem(World* world)
	: System(world)
{
}

void MeleeAttackSystem::Update(float dt)
{
	(void)dt;

	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvMeleeAttackRequest>([&](const EvMeleeAttackRequest& request)
		{
			ProcessMeleeAttack(request);
		});
}

void MeleeAttackSystem::ProcessMeleeAttack(const EvMeleeAttackRequest& request)
{
	if (!request.shooter.IsValid())
		return;

	const bool attackerIsPlayer = mWorld->HasComponent<MainPlayerComponent>(request.shooter);
	const bool attackerIsEnemy = mWorld->HasComponent<EnemyComponent>(request.shooter);
	if (!attackerIsPlayer && !attackerIsEnemy)
		return;

	TransformComponent* attackerTransform = mWorld->GetComponent<TransformComponent>(request.shooter);
	InputComponent* attackerInput = mWorld->GetComponent<InputComponent>(request.shooter);
	if (!attackerTransform || !attackerInput)
		return;

	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	const MeleeAttackStat stat = GetMeleeAttackStat(request.bulletType);
	const Vec3 forward = GetCameraForwardFromInput(*attackerInput);
	const Vec3 attackCenter = attackerTransform->mWorldPosition + forward * stat.forwardDistance;
	const float radiusSq = stat.radius * stat.radius;
	const Vec3 attackerRotation = attackerTransform->mLocalRotationE;

	eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
		static_cast<uint8>(request.bulletType),
		attackCenter.x,
		attackCenter.y,
		attackCenter.z,
		EffectSpawnReason::Fire,
		attackerRotation.x,
		attackerRotation.y,
		attackerRotation.z
		});

	auto candidates = mWorld->GetEntitiesWithComponents<TransformComponent, HealthComponent>();
	for (Entity target : candidates)
	{
		if (!target.IsValid() || target == request.shooter)
			continue;

		const bool targetIsPlayer = mWorld->HasComponent<MainPlayerComponent>(target);
		const bool targetIsEnemy = mWorld->HasComponent<EnemyComponent>(target);
		if (attackerIsPlayer && !targetIsEnemy)
			continue;
		if (attackerIsEnemy && !targetIsPlayer)
			continue;


		TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(target);
		if (!targetTransform)
			continue;

		Vec3 delta = targetTransform->mWorldPosition - attackCenter;
		delta.y = 0.0f;
		if (delta.LengthSquared() > radiusSq)
			continue;

		if (stat.knockbackDistance > 0.0f)
		{
			Vec3 knockbackDirection = targetTransform->mWorldPosition - attackerTransform->mWorldPosition;
			knockbackDirection.y = 0.0f;

			if (knockbackDirection.LengthSquared() > 1e-6f)
			{
				knockbackDirection.Normalize();
				const Vec3 knockbackVector = knockbackDirection * stat.knockbackDistance;
				targetTransform->mLocalPosition += knockbackVector;
				targetTransform->mMovingVector += knockbackVector;
			}
		}

		BuffComponent* attackerBuff = mWorld->GetComponent<BuffComponent>(request.shooter);
		const float attackMultiplier = attackerBuff ? attackerBuff->mAttackMultiplier : 1.0f;

		EvDamage damageEvent{};
		damageEvent.instigator = request.shooter;
		damageEvent.target = target;
		damageEvent.amount = static_cast<int32>((std::max)(0.0f, stat.damage * attackMultiplier));
		eventManager->Enqueue<EvDamage>(damageEvent);

		eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
			static_cast<uint8>(request.bulletType),
			targetTransform->mWorldPosition.x,
			targetTransform->mWorldPosition.y,
			targetTransform->mWorldPosition.z,
			EffectSpawnReason::CollisionEntity,
			attackerRotation.x,
			attackerRotation.y,
			attackerRotation.z });
	}
}