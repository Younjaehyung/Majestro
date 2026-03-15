#include "pch.h"
#include "MeleeAttackSystem.h"

#include "World.h"
#include "EventManager.h"
#include "TransformComponent.h"
#include "InputComponent.h"
#include "HealthComponent.h"
#include "BulletComponent.h"

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
	};

	MeleeAttackStat GetMeleeAttackStat(SkillType type)
	{
		switch (type)
		{
		case SkillType::DrumAttack:
			return { 10.0f, 3.0f, 100.0f };
		case SkillType::GuitarAttack:
			return { 25.0f, 3.0f, 2.2f };
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

	auto candidates = mWorld->GetEntitiesWithComponents<TransformComponent, HealthComponent>();
	for (Entity target : candidates)
	{
		if (!target.IsValid() || target == request.shooter)
			continue;

		TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(target);
		if (!targetTransform)
			continue;

		Vec3 delta = targetTransform->mWorldPosition - attackCenter;
		delta.y = 0.0f;
		if (delta.LengthSquared() > radiusSq)
			continue;

		EvDamage damageEvent{};
		damageEvent.instigator = request.shooter;
		damageEvent.target = target;
		damageEvent.amount = static_cast<int32>((std::max)(0.0f, stat.damage));
		eventManager->Enqueue<EvDamage>(damageEvent);
	}
}