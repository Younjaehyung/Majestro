#include "pch.h"
#include "BulletFireEventSystem.h"
#include "World.h"
#include "TransformComponent.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "ColliderComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "MovementComponent.h"
#include "BuffComponent.h"
#include "PhysicsWorld.h"
#include "ServerCore.h"
#include "GameEvents.h"


	
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
				ActivateBulletAndNotify(e.shooter, e.bulletType, e.isCritical);
			});
	}
}

void BulletFireEventSystem::ActivateBulletAndNotify(Entity playerEntity, SkillType bulletType, bool isCritical)
{
	if (false == mWorld->HasComponentPool<BulletComponent>())
		return;

	TransformComponent* shooterTransform = mWorld->GetComponent<TransformComponent>(playerEntity);
	NetEntityComponent* shooterNetComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(playerEntity);
	BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(playerEntity);
	const bool shooterIsPlayer = mWorld->HasComponent<MainPlayerComponent>(playerEntity);
	const bool shooterIsEnemy = mWorld->HasComponent<EnemyComponent>(playerEntity);

	if (shooterTransform == nullptr || shooterNetComp == nullptr)
		return;
	if (!shooterIsPlayer && !shooterIsEnemy)
		return;

	auto rotateYaw = [](const Vec3& baseDir, float degrees)
	{
		const float yawRad = DirectX::XMConvertToRadians(degrees);
		const float cosYaw = std::cos(yawRad);
		const float sinYaw = std::sin(yawRad);

		Vec3 rotated;
		rotated.x = baseDir.x * cosYaw - baseDir.z * sinYaw;
		rotated.y = baseDir.y;
		rotated.z = baseDir.x * sinYaw + baseDir.z * cosYaw;
		if (rotated.LengthSquared() <= 0.0001f)
			return baseDir;
		rotated.Normalize();
		return rotated;
	};

			auto activateSingleBullet = [&](const Vec3& spawnPosition, const Vec3& direction)
		{
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

				BulletStat bulletStat = GetBulletStat(bulletType);
				const float attackMultiplier = buffComp ? buffComp->mAttackMultiplier : 1.0f;
				bulletStat.Damage *= attackMultiplier;
				if (shooterIsPlayer)
				{
					if (MainPlayerComponent* shooterPlayer = mWorld->GetComponent<MainPlayerComponent>(playerEntity))
					{
						if (shooterPlayer->mPlayerType == Ibanix &&
							(shooterPlayer->mNowBullet + 1) >= 11)
						{
							bulletStat.Damage *= 2.0f;
						}
					}
				}
				if (isCritical)
					bulletStat.Damage *= 2.0f;

			bulletTransform->mWorldPosition = spawnPosition;
			bulletTransform->mLocalPosition = bulletTransform->mWorldPosition;
			bulletTransform->mLocalScale = Vec3(bulletStat.Size, bulletStat.Size, bulletStat.Size);
			bulletTransform->mMovingVector = Vec3::Zero;
			bulletTransform->LookAt(direction);

			if (BoxColliderComponent* bulletCollider = mWorld->GetComponent<BoxColliderComponent>(bulletEntity))
			{
				const float halfSize = bulletStat.Size * 0.5f;
				bulletCollider->SetBox(Vec3(halfSize, halfSize, halfSize), Vec3::Zero);
			}

			const uint16 generation = static_cast<uint16>(bulletComp->mGeneration + 1);
			bulletComp->mPenetrates = bulletStat.Penetrates;
			bulletComp->mPenetratesStatic = bulletStat.PenetratesStatic;
			bulletComp->Activate(bulletType, shooterNetComp->mNetEntityId, static_cast<uint32>(bulletNetComp->mNetEntityId), generation, direction, bulletStat.Speed, bulletStat.LifeTime, bulletStat.Damage, bulletStat.KnockbackDistance, isCritical);
			mWorld->RegisterActiveBullet(bulletEntity);

				if (bulletType != SkillType::BrassSkill3)
				{
					if (auto eventManager = mWorld->GetEventManager())
					{
						eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
							static_cast<uint8>(bulletType),
							bulletTransform->mWorldPosition.x,
							bulletTransform->mWorldPosition.y,
							bulletTransform->mWorldPosition.z,
							EffectSpawnReason::Fire,
							bulletTransform->mLocalRotationE.x,
							bulletTransform->mLocalRotationE.y,
							bulletTransform->mLocalRotationE.z });
					}
				}

			S2C_BulletActivatePacket bulletPacket{};
			bulletPacket.SendTime = std::chrono::duration<double>(
				std::chrono::system_clock::now().time_since_epoch()).count();
			bulletPacket.ownerNetEntityId = shooterNetComp->mNetEntityId;
			bulletPacket.bulletNetEntityId = bulletNetComp->mNetEntityId;
			bulletPacket.bulletGeneration = bulletComp->mGeneration;
			bulletPacket.bulletType = static_cast<uint8>(bulletType);
			bulletPacket.x = bulletTransform->mWorldPosition.x;
			bulletPacket.y = bulletTransform->mWorldPosition.y;
			bulletPacket.z = bulletTransform->mWorldPosition.z;
			bulletPacket.dirX = direction.x;
			bulletPacket.dirY = direction.y;
			bulletPacket.dirZ = direction.z;
			bulletPacket.rotX = bulletTransform->mLocalRotationE.x;
			bulletPacket.rotY = bulletTransform->mLocalRotationE.y;
			bulletPacket.rotZ = bulletTransform->mLocalRotationE.z;
			bulletPacket.speed = bulletComp->mSpeed;
			bulletPacket.lifeTime = bulletComp->mLifeTime;
			bulletPacket.size = bulletStat.Size;

			auto recipients = CollectPlayerSessions();
			for (uint32 sessionId : recipients)
			{
				SendRequest request{ sessionId, PKT_Type::S2C_PKT_BULLET_ACTIVATE, sizeof(S2C_BulletActivatePacket) };
				request.StoreAs<S2C_BulletActivatePacket>(bulletPacket);
				gSendQueue.Push(request);
			}

			return true;
		}

		return false;
	};

	if (shooterIsEnemy && bulletType == SkillType::BrassSkill4)
	{
		const Vec3 fixedSpawnPositions[3] = {
			Vec3(-3500.0f, 200.0f,    0.0f),
			Vec3(-3500.0f, 200.0f, -470.0f),
			Vec3(-3500.0f, 200.0f,  470.0f),
		};
		const Vec3 sharedDir = Vec3::Right;

		for (const Vec3& spawnPosition : fixedSpawnPositions)
		{
			activateSingleBullet(spawnPosition, sharedDir);
		}
		return;
	}

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

		Vec3 direction = Vec3::Forward;
		Vec3 spawnPosition = shooterTransform->mWorldPosition + Vec3::Up * 90.0f;
		if (shooterIsPlayer)
		{
			if (inputComp == nullptr)
				return;

			
			Vec3 cameraForward = GetCameraForwardFromInput(*inputComp);
			Vec3 cameraPosition = CalculateServerTpsCameraPosition(*shooterTransform, cameraForward);
			if (inputComp->HasAimCameraRay)
			{
				cameraPosition = inputComp->AimCameraPosition;
				cameraForward = inputComp->AimCameraDirection;
				if (cameraForward.LengthSquared() <= 0.0001f)
					cameraForward = GetCameraForwardFromInput(*inputComp);
				else
					cameraForward.Normalize();
			}
			Vec3 aimPoint = cameraPosition + cameraForward * kMaxAimDistance;
			spawnPosition = CalculateServerMuzzlePosition(*shooterTransform, cameraForward);

			if (auto physicsWorld = mWorld->GetPhysicsWorld())
			{
				
				physicsWorld->QueryAimPoint(playerEntity, cameraPosition, cameraForward, kMaxAimDistance, aimPoint);

				JoltStaticHit muzzleBlock{};
				const Vec3 muzzleToAim = aimPoint - spawnPosition;
				if (muzzleToAim.LengthSquared() > 0.0001f && physicsWorld->RayCastStatic(spawnPosition, aimPoint, muzzleBlock))
				{
					aimPoint = muzzleBlock.point;
				}
			}

			direction = aimPoint - spawnPosition;
			if (direction.LengthSquared() <= 0.0001f)
				direction = cameraForward;
			else
				direction.Normalize();
		}
		else
		{
			if (bulletType == SkillType::BrassSkill3)
			{
				direction = shooterTransform->GetLook();
			}
			else
			{
				float nearestDistSq = (std::numeric_limits<float>::max)();
				Vec3 nearestPlayerPosition = shooterTransform->mWorldPosition;
				for (auto playerTarget : mWorld->GetEntitiesWithComponents<MainPlayerComponent, TransformComponent>())
				{
					TransformComponent* playerTargetTransform = mWorld->GetComponent<TransformComponent>(playerTarget);
					if (!playerTargetTransform)
						continue;

					const float distSq = Vec3::DistanceSquared(shooterTransform->mWorldPosition, playerTargetTransform->mWorldPosition);
					if (distSq < nearestDistSq)
					{
						nearestDistSq = distSq;
						nearestPlayerPosition = playerTargetTransform->mWorldPosition;
					}
				}

				direction = nearestPlayerPosition - shooterTransform->mWorldPosition;
			}
			if (direction.LengthSquared() <= 0.0001f)
				direction = shooterTransform->GetLook();
			if (direction.LengthSquared() <= 0.0001f)
				direction = Vec3::Forward;
			direction.Normalize();
			spawnPosition = shooterTransform->mWorldPosition + direction * 3.0f + Vec3(0.f, 90.f, 0.f);
		}
			if (bulletType == SkillType::BaseAttack2)
			{
				activateSingleBullet(spawnPosition, rotateYaw(direction, -20.0f));
				activateSingleBullet(spawnPosition, direction);
				activateSingleBullet(spawnPosition, rotateYaw(direction, 20.0f));
			}
			else
			{
				activateSingleBullet(spawnPosition, direction);
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


Vec3 BulletFireEventSystem::GetCameraForwardFromInput(const InputComponent& input)
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

Vec3 BulletFireEventSystem::SafeHorizontalForward(Vec3 forward)
{
	forward.y = 0.0f;
	if (forward.LengthSquared() <= 0.0001f)
		return Vec3::Forward;

	forward.Normalize();
	return forward;
}

Vec3 BulletFireEventSystem::SafeRightFromForward(const Vec3& forward)
{
	Vec3 right = Vec3::Up.Cross(SafeHorizontalForward(forward));
	if (right.LengthSquared() <= 0.0001f)
		return Vec3::Right;

	right.Normalize();
	return right;
}

Vec3 BulletFireEventSystem::CalculateServerTpsCameraPosition(const TransformComponent& shooterTransform, const Vec3& cameraForward)
{

	const Vec3 yawRight = SafeRightFromForward(cameraForward);
	const Vec3 pivot = shooterTransform.mWorldPosition
		+ yawRight * kCameraRightOffset
		+ Vec3::Up * kCameraUpOffset;


	return pivot - cameraForward * kCameraBackDistance;
}

Vec3 BulletFireEventSystem::CalculateServerMuzzlePosition(const TransformComponent& shooterTransform, const Vec3& cameraForward)
{

	const Vec3 yawForward = SafeHorizontalForward(cameraForward);
	const Vec3 yawRight = SafeRightFromForward(cameraForward);

	
	return shooterTransform.mWorldPosition + yawRight * kMuzzleRightOffset
		+ Vec3::Up * kMuzzleUpOffset+ yawForward * kMuzzleForwardOffset;
}
