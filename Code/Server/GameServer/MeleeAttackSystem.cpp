#include "pch.h"
#include "MeleeAttackSystem.h"

#include "World.h"
#include "EventManager.h"
#include "TransformComponent.h"
#include "InputComponent.h"
#include "HealthComponent.h"
#include "BulletComponent.h"
#include "BuffComponent.h"
#include "RhythmComponents.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "MovementComponent.h"
#include "BeatSystem.h"
#include "PhysicsWorld.h"
#include "MathUtils.h"

namespace
{
	constexpr float kSilenceDurationMeasures = 2.0f;
	constexpr float kBeatsPerMeasure = 4.0f;

	Vec3 GetCameraForwardFromInput(const InputComponent& input)
	{
		return MathUtils::ForwardFromYawPitchDegrees(input.Yaw, input.Pitch);
	}

	// 크로스헤어 조준 상수들
	constexpr float kMaxAimDistance     = 5000.0f;
	constexpr float kCameraRightOffset  = 80.0f;
	constexpr float kCameraUpOffset     = 160.0f;
	constexpr float kCameraBackDistance = 250.0f;
	constexpr float kMuzzleRightOffset  = 35.0f;
	constexpr float kMuzzleUpOffset     = 110.0f;
	constexpr float kMuzzleForwardOffset = 5.0f;


	Vec3 TpsCameraPos(const TransformComponent& t, const Vec3& camF)
	{
		const Vec3 right = MathUtils::SafeRightFromForward(camF);
		const Vec3 pivot = t.mWorldPosition + right * kCameraRightOffset + Vec3::Up * kCameraUpOffset;
		return pivot - camF * kCameraBackDistance;
	}

	Vec3 MuzzlePos(const TransformComponent& t, const Vec3& camF)
	{
		const Vec3 yawForward = MathUtils::SafeHorizontalForward(camF);
		const Vec3 right      = MathUtils::SafeRightFromForward(camF);
		return t.mWorldPosition + right * kMuzzleRightOffset + Vec3::Up * kMuzzleUpOffset + yawForward * kMuzzleForwardOffset;
	}

	// 총구에서 크로스헤어가 가리키는 월드 지점(aimPoint)까지의 방향. 3인칭 시차를 보정한다.
	Vec3 ComputeCrosshairForward(World* world, Entity shooter, const TransformComponent& t, const InputComponent& input)
	{
		Vec3 camF = GetCameraForwardFromInput(input);
		Vec3 camP = TpsCameraPos(t, camF);
		if (input.HasAimCameraRay)
		{
			camP = input.AimCameraPosition;
			Vec3 d = input.AimCameraDirection;
			if (d.LengthSquared() > 0.0001f)
			{
				d.Normalize();
				camF = d;
			}
		}

		const Vec3 muzzle = MuzzlePos(t, camF);
		Vec3 aimPoint = camP + camF * kMaxAimDistance;
		if (auto physicsWorld = world->GetPhysicsWorld())
			physicsWorld->QueryAimPoint(shooter, camP, camF, kMaxAimDistance, aimPoint);

		Vec3 dir = aimPoint - muzzle;
		if (dir.LengthSquared() <= 0.0001f)
			return camF;
		dir.Normalize();
		return dir;
	}

	Vec3 GetEnemyAttackForward(World* world, Entity attacker, TransformComponent& attackerTransform)
	{
		if (!world || !world->HasComponentPool<PlayerMovementComponent>())
		{
			Vec3 fallback = attackerTransform.GetLook();
			fallback.y = 0.0f;
			if (fallback.LengthSquared() <= 0.0001f)
				return Vec3::Forward;

			fallback.Normalize();
			return fallback;
		}

		const EnemyComponent* enemy = world->GetComponent<EnemyComponent>(attacker);
		if (enemy && enemy->mBossPolicyTarget.IsValid())
		{
			const Entity target = enemy->mBossPolicyTarget;
			const MainPlayerComponent* player = world->GetComponent<MainPlayerComponent>(target);
			const HealthComponent* health = world->GetComponent<HealthComponent>(target);
			const TransformComponent* targetTransform = world->GetComponent<TransformComponent>(target);
			if (player && !player->IsDeathActive() && (!health || health->mCurrentHp > 0) && targetTransform)
			{
				Vec3 direction = targetTransform->mWorldPosition - attackerTransform.mWorldPosition;
				direction.y = 0.0f;
				if (direction.LengthSquared() > 0.0001f)
				{
					direction.Normalize();
					return direction;
				}
			}
		}

		Vec3 nearestDirection = Vec3::Zero;
		float nearestDistanceSq = (std::numeric_limits<float>::max)();

		for (const Entity& playerEntity : world->GetEntitiesWithComponent<PlayerMovementComponent>())
		{
			if (!playerEntity.IsValid() || playerEntity == attacker)
				continue;

			TransformComponent* playerTransform = world->GetComponent<TransformComponent>(playerEntity);
			if (!playerTransform)
				continue;

			Vec3 direction = playerTransform->mWorldPosition - attackerTransform.mWorldPosition;
			direction.y = 0.0f;

			const float distanceSq = direction.LengthSquared();
			if (distanceSq <= 0.0001f || distanceSq >= nearestDistanceSq)
				continue;

			nearestDistanceSq = distanceSq;
			nearestDirection = direction;
		}

		if (nearestDirection.LengthSquared() <= 0.0001f)
		{
			Vec3 fallback = attackerTransform.GetLook();
			fallback.y = 0.0f;
			if (fallback.LengthSquared() <= 0.0001f)
				return Vec3::Forward;

			fallback.Normalize();
			return fallback;
		}

		nearestDirection.Normalize();
		return nearestDirection;
	}

	struct MeleeAttackStat
	{
		float damage = 10.0f;
		float forwardDistance = 3.0f;
		float radius = 2.0f;
		float angleDegrees = 360.0f;
		float knockbackDistance = 0.0f;
	};

	MeleeAttackStat GetMeleeAttackStat(SkillType type)
	{
		switch (type)
		{
			case SkillType::DrumAttack:
				return { 25.0f, 3.0f, 500.0f, 150.0f, 0.0f };
			case SkillType::DrumAttack3:
				return { 30.0f, 3.0f, 500.0f, 150.0f, 0.0f };
			case SkillType::DrumSkill1:
				return { 55.0f, 3.0f, 500.0f, 360.0f, 0.0f };
		case SkillType::GuitarAttack:
			return { 25.0f, 3.0f, 500.0f, 150.0f, 0.0f };
		case SkillType::GuitarSkill1:
			return { 75.0f, 3.0f, 800.0f, 150.0f, 0.0f };
		case SkillType::PianoAttack:
			return { 30.0f, 3.0f, 200.0f, 360.0f, 0.0f };
		case SkillType::SlimeAttack:
			return { 20.0f, 3.0f, 500.0f, 360.0f, 0.0f };
		case SkillType::FlyAttack:
			return { 20.0f, 3.0f, 500.0f, 360.0f, 0.0f };
		case SkillType::BongoAttack:
			return { 60.0f, 3.0f, 500.0f, 360.0f, 0.0f };
		case SkillType::DragonSkill1:
			return { 30.0f, 4.0f, 1000.0f, 360.0f, 0.0f };
			case SkillType::DragonSkill3:
				return { 50.0f, 200.0f, 300.0f, 360.0f, 600.0f };
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
	if (!attackerTransform)
		return;

	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	MeleeAttackStat stat = GetMeleeAttackStat(request.bulletType);

	MainPlayerComponent* attackerPlayer = attackerIsPlayer
		? mWorld->GetComponent<MainPlayerComponent>(request.shooter) : nullptr;

	const RhythmEffectComponent* attackerRhythmEffect = attackerIsPlayer
		? mWorld->GetComponent<RhythmEffectComponent>(request.shooter) : nullptr;


	if (attackerPlayer && attackerRhythmEffect)
	{
		const RhythmVariantEffectModifiers& modifiers = attackerRhythmEffect->GetVariantModifiers();
		stat.damage += static_cast<float>(modifiers.attackPowerBonus);
		stat.radius *= modifiers.rhythmEffectRangeMultiplier;
		if (attackerPlayer->mPlayerType == Fanthor)
		{
			stat.damage *= modifiers.outgoingRhythmEffectMultiplier;
			stat.knockbackDistance *= modifiers.outgoingRhythmEffectMultiplier;
		}
	}


	Vec3 forward = Vec3::Forward;
	const EnemyComponent* attackerEnemy = attackerIsEnemy ? mWorld->GetComponent<EnemyComponent>(request.shooter) : nullptr;
	const bool attackerIsFly = attackerEnemy && attackerEnemy->mEnemyType == EnemyType::Fly;

	if (attackerIsPlayer)
	{
		InputComponent* attackerInput = mWorld->GetComponent<InputComponent>(request.shooter);
		if (!attackerInput)
			return;

		forward = GetCameraForwardFromInput(*attackerInput);
	}
	else
	{
		forward = GetEnemyAttackForward(mWorld, request.shooter, *attackerTransform);
	}

	const Vec3 attackCenter = attackerTransform->mWorldPosition + forward * stat.forwardDistance;
	const float radiusSq = stat.radius * stat.radius;


	Vec3 effectForward = forward;
	if (attackerIsPlayer)
	{
		if (const InputComponent* effectInput = mWorld->GetComponent<InputComponent>(request.shooter))
			effectForward = ComputeCrosshairForward(mWorld, request.shooter, *attackerTransform, *effectInput);
	}



	const Vec3 attackerRotation = MathUtils::EulerDegreesFromForward(effectForward);

		if (request.bulletType != SkillType::PianoAttack)
		{
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
		}

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
		if (targetIsEnemy)
		{
			HealthComponent* targetHealth = mWorld->GetComponent<HealthComponent>(target);
			if (targetHealth && targetHealth->mCurrentHp <= 0)
				continue;
		}


		TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(target);
		if (!targetTransform)
			continue;

		Vec3 delta = targetTransform->mWorldPosition - attackCenter;
		delta.y = 0.0f;
		if (delta.LengthSquared() > radiusSq)
			continue;

		if (attackerIsPlayer && stat.angleDegrees < 359.9f)
		{
			Vec3 flatForward = forward;
			flatForward.y = 0.0f;
			if (flatForward.LengthSquared() <= 1e-6f)
				flatForward = Vec3::Forward;
			else
				flatForward.Normalize();

			Vec3 toTarget = delta;
			if (toTarget.LengthSquared() <= 1e-6f)
				continue;

			toTarget.Normalize();
			const float halfAngleDegrees = stat.angleDegrees * 0.5f;
			const float minDot = std::cos(DirectX::XMConvertToRadians(halfAngleDegrees));
			if (flatForward.Dot(toTarget) < minDot)
				continue;
		}

		if (stat.knockbackDistance > 0.0f)
		{
			Vec3 knockbackDirection = targetTransform->mWorldPosition - attackerTransform->mWorldPosition;
			knockbackDirection.y = 0.0f;

			if (knockbackDirection.LengthSquared() > 1e-6f)
			{
				knockbackDirection.Normalize();
				const Vec3 knockbackVector = knockbackDirection * stat.knockbackDistance;
				if (request.bulletType == SkillType::DragonSkill3 && targetIsPlayer)
				{
					eventManager->Enqueue<EvImpulse>({
						target,
						knockbackVector.x,
						0.0f,
						knockbackVector.z,
						ImpulseSource::Knockback
					});
				}
				else
				{
					targetTransform->mLocalPosition += knockbackVector;
					targetTransform->mMovingVector += knockbackVector;
				}
			}
		}

		BuffComponent* attackerBuff = mWorld->GetComponent<BuffComponent>(request.shooter);
		const float attackMultiplier = attackerBuff ? attackerBuff->mAttackMultiplier : 1.0f;

		EvDamage damageEvent{};
		damageEvent.instigator = request.shooter;
		damageEvent.target = target;
		damageEvent.skillType = request.bulletType;
		damageEvent.amount = static_cast<int32>((std::max)(0.0f, stat.damage * attackMultiplier));
		damageEvent.isCritical = request.isCritical;
		damageEvent.isOnBeat = request.isOnBeat;
		if (damageEvent.isCritical)
			damageEvent.amount *= 2;
		eventManager->Enqueue<EvDamage>(damageEvent);

		if ((attackerIsFly || request.bulletType == SkillType::DragonSkill3) && targetIsPlayer)
		{
			BuffComponent* targetBuff = mWorld->GetComponent<BuffComponent>(target);
			if (targetBuff)
			{
				float beatSeconds = 0.0f;
				if (auto systemManager = mWorld->GetSystemManager())
				{
					if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
						beatSeconds = beatSystem->mBpmSeconds;
				}

				BuffData silence{};
				silence.mKind = EffectKind::Debuff;
				silence.mType = BuffType::Silence;
				silence.mDurationPolicy = DurationPolicy::Timed;
				silence.mExecutionType = BuffExecutionType::Persistent;
				silence.mSource = request.shooter;
				silence.mEndTime = GetServerTotalTimeSeconds() + beatSeconds * kSilenceDurationMeasures * kBeatsPerMeasure;
				targetBuff->AddOrRefresh(silence);
			}
		}

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
