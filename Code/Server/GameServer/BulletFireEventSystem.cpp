#include "pch.h"
#include "BulletFireEventSystem.h"

#include <chrono>
#include <unordered_set>

#include "World.h"
#include "TransformComponent.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "ColliderComponent.h"
#include "PlayerComponent.h"
#include "ServerCore.h"
#include "GameEvents.h"

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
}

BulletFireEventSystem::BulletFireEventSystem(World* world)
	: System(world)
{
}

void BulletFireEventSystem::Update(float dt)
{
	if (auto eventManager = mWorld->GetEventManager())
	{

		eventManager->Consume<EvRangedAttackRequest>([&](const EvRangedAttackRequest& e)
			{
				ActivateBulletAndNotify(e.shooter, e.bulletType);
			});

		eventManager->Consume<EvBuffBulletRequest>([&](const EvBuffBulletRequest& e)
			{
				ActivateBulletAndNotify(e.shooter, e.bulletType);
			});
	}
}

void BulletFireEventSystem::ActivateBulletAndNotify(Entity playerEntity, SkillType bulletType)
{
	if (false == mWorld->HasComponentPool<BulletComponent>())
		return;

	TransformComponent* playerTransform = mWorld->GetComponent<TransformComponent>(playerEntity);
	NetEntityComponent* playerNetComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(playerEntity);
	if (playerTransform == nullptr || playerNetComp == nullptr || inputComp == nullptr)
		return;

	auto bulletEntities = mWorld->GetEntitiesWithComponents<BulletComponent, TransformComponent, NetEntityComponent>();
	for (auto bulletEntity : bulletEntities)
	{
		BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
		if (bulletComp == nullptr || bulletComp->mIsActive)
			continue;

		TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
		NetEntityComponent* bulletNetComp = mWorld->GetComponent<NetEntityComponent>(bulletEntity);
		if (bulletTransform == nullptr || bulletNetComp == nullptr)
			continue;

		Vec3 direction = GetCameraForwardFromInput(*inputComp);
		const BulletStat bulletStat = GetBulletStat(bulletType);

		bulletTransform->mWorldPosition = playerTransform->mWorldPosition + direction * 3.0f + Vec3(0.f, 90.f, 0.f);
		bulletTransform->mLocalPosition = bulletTransform->mWorldPosition;
		bulletTransform->mLocalScale = Vec3(bulletStat.Size, bulletStat.Size, bulletStat.Size);
		bulletTransform->mMovingVector = direction * bulletStat.Speed;

		if (BoxColliderComponent* bulletCollider = mWorld->GetComponent<BoxColliderComponent>(bulletEntity))
		{
			const float halfSize = bulletStat.Size * 0.5f;
			bulletCollider->SetBox(Vec3(halfSize, halfSize, halfSize), Vec3::Zero);
		}

		const uint16 generation = static_cast<uint16>(bulletComp->mGeneration + 1);
		bulletComp->mPenetrationCount = bulletStat.PenetrationCount;
		bulletComp->Activate(bulletType, playerNetComp->mNetEntityId, static_cast<uint32>(bulletNetComp->mNetEntityId), generation, direction, bulletStat.Speed, bulletStat.LifeTime, bulletStat.Damage, bulletStat.KnockbackDistance);

		mWorld->RegisterActiveBullet(bulletEntity);

		S2C_BulletActivatePacket bulletPacket{};
		bulletPacket.SendTime = std::chrono::duration<double>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		bulletPacket.ownerNetEntityId = playerNetComp->mNetEntityId;
		bulletPacket.bulletNetEntityId = bulletNetComp->mNetEntityId;
		bulletPacket.bulletType = static_cast<uint8>(bulletType);
		bulletPacket.x = bulletTransform->mWorldPosition.x;
		bulletPacket.y = bulletTransform->mWorldPosition.y;
		bulletPacket.z = bulletTransform->mWorldPosition.z;
		bulletPacket.dirX = direction.x;
		bulletPacket.dirY = direction.y;
		bulletPacket.dirZ = direction.z;
		bulletPacket.speed = bulletComp->mSpeed;

		auto recipients = CollectPlayerSessions();
		for (uint32 sessionId : recipients)
		{
			SendRequest request{ sessionId, PKT_Type::S2C_PKT_BULLET_ACTIVATE, sizeof(S2C_BulletActivatePacket) };
			request.StoreAs<S2C_BulletActivatePacket>(bulletPacket);
			gSendQueue.Push(request);
		}

		return;
	}
}

std::vector<uint32> BulletFireEventSystem::CollectPlayerSessions() const
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())
		return {};

	std::unordered_set<uint32> sessionSet;
	auto players = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	sessionSet.reserve(players.size());

	for (auto entity : players)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr || netComp->mSessionId == 0)
			continue;

		sessionSet.insert(netComp->mSessionId);
	}

	return std::vector<uint32>(sessionSet.begin(), sessionSet.end());
}