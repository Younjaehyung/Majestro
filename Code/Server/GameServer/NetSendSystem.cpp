#include "pch.h"
#include "NetSendSystem.h"
#include "World.h"
#include "NetEntityComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "ColliderComponent.h"
#include "MovementComponent.h"
#include "BulletComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "GameRuleComponent.h"
#include "GameEvents.h"
#include "EventManager.h"
#include "InteractableComponent.h"
#include "SpawnerComponent.h"
#include "TruckComponent.h"


NetSendSystem::NetSendSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

void NetSendSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;


	HandleSessionJoinedEvents();   // 신규 세션 초기 상태 송신
	SendAction();
	SendCollision();
	SendHealthEvents();
	SendArmorEvents();
	SendAmmoEvents();
	SendBulletDeactivateEvents();
	SendEffectSpawnEvents();
	SendHitConfirmEvents();


	if (mMovementRate.Tick(dt))           // 30Hz 주기 전송 (UDP)
		SendMove(dt);	//move


}

void NetSendSystem::SendMove(float dt)
{

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		if (mWorld->HasComponent<BulletComponent>(entity))
			continue;
		{

			TransformComponent* transComp = mWorld->GetComponent<TransformComponent>(netComp->mOwnerEntity);

			S2C_MovePacket movePkt;

			movePkt.netEntityId = netComp->mNetEntityId;
			movePkt.x = transComp->mWorldPosition.x;
			movePkt.y = transComp->mWorldPosition.y;
			movePkt.z = transComp->mWorldPosition.z;

			movePkt.vx = transComp->mMovingVector.x;
			movePkt.vy = transComp->mMovingVector.y;
			movePkt.vz = transComp->mMovingVector.z;
			
			movePkt.rx = transComp->mLocalRotationQ.x;
			movePkt.ry = transComp->mLocalRotationQ.y;
			movePkt.rz = transComp->mLocalRotationQ.z;
			movePkt.rw = transComp->mLocalRotationQ.w;


			movePkt.yaw = transComp->mLocalRotationE.y;
			movePkt.pitch = transComp->mLocalRotationE.x;

			Broadcast(recipients, S2C_PKT_MOVE, movePkt);
		}
	}


}


void NetSendSystem::SendAction()
{
	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	if (mWorld->HasComponentPool<MainPlayerComponent>())
	{
		std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
		for (auto& entity : entities)
		{

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(netComp->mOwnerEntity);
			S2C_StatePacket statePkt;
			statePkt.netEntityId = netComp->mNetEntityId;
			statePkt.stateId = playerComp->GetReplicatedActionState();
			statePkt.lowerStateId = playerComp->GetReplicatedMovementMode();
			statePkt.controlFlags = playerComp->GetReplicatedControlFlags();
			statePkt.externalMoveMode = playerComp->GetReplicatedExternalMoveMode();
			statePkt.stateSequence = playerComp->mStateSequence;

			Broadcast(recipients, S2C_PKT_STATE, statePkt);
		}
	}

	if (mWorld->HasComponentPool<EnemyComponent>())
	{
		std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, EnemyComponent>();
		for (auto& entity : entities)
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(netComp->mOwnerEntity);
			if (!netComp || !enemyComp)
				continue;

			S2C_StatePacket statePkt;
			statePkt.netEntityId = netComp->mNetEntityId;
			statePkt.stateId = enemyComp->mAnimState;
			statePkt.lowerStateId = enemyComp->mAnimState;
			statePkt.stateSequence = 0;

			Broadcast(recipients, S2C_PKT_STATE, statePkt);
		}
	}
}

void NetSendSystem::SendCollision()
{
	if (false == mWorld->HasComponentPool<BoxColliderComponent>())return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<BoxColliderComponent, NetEntityComponent>();
	for (auto& entity : entities)
	{
		//if (netComp->mIsDirty)
		{

			/*mSendReq.SessionId = 0;
			mSendReq.Type = S2C_PKT_STATE;
			mSendReq.Size = sizeof(S2C_CollisionPacket);*/
			S2C_CollisionPacket collisionPkt;
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			collisionPkt.netEntityId = netComp->mNetEntityId;
			collisionPkt.bIsColliding = mWorld->GetComponent<BoxColliderComponent>(entity)->bIsColliding;

			Broadcast(recipients, S2C_PKT_COLLISION, collisionPkt);
		}
	}
}


std::vector<uint32> NetSendSystem::CollectPlayerSessions()
{

	mSessionSet.clear();

	if (false == mWorld->HasComponentPool<NetEntityComponent>())
		return {};

	auto entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	mSessionSet.reserve(entities.size());
	for (auto entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (!netComp || netComp->mSessionId == 0)
			continue;

		mSessionSet.insert(netComp->mSessionId);
	}

	return std::vector<uint32>(mSessionSet.begin(), mSessionSet.end());
}



void NetSendSystem::SendHealthEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	eventManager->Consume<EvHealthChanged>([&](const EvHealthChanged& e)
		{
			if (!e.target.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.target);
			if (netComp == nullptr)
				return;

			S2C_HealthPacket healthPkt;
			healthPkt.netEntityId = netComp->mNetEntityId;
			healthPkt.currentHp = e.currentHp;
			healthPkt.maxHp = e.maxHp;

			// 어택커 NetEntityId (없으면 0) — 클라이언트가 hit direction 계산에 사용.
			if (e.instigator.IsValid())
			{
				if (NetEntityComponent* atkNet = mWorld->GetComponent<NetEntityComponent>(e.instigator))
					healthPkt.attackerNetId = atkNet->mNetEntityId;
			}

			Broadcast(recipients, S2C_PKT_HEALTH, healthPkt);
		});
}

void NetSendSystem::SendArmorEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	eventManager->Consume<EvArmorChanged>([&](const EvArmorChanged& e)
		{
			if (!e.target.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.target);
			if (netComp == nullptr)
				return;

			S2C_ArmorPacket armorPkt;
			armorPkt.netEntityId = netComp->mNetEntityId;
			armorPkt.currentArmor = e.currentArmor;
			armorPkt.maxArmor = e.maxArmor;

			Broadcast(recipients, S2C_PKT_ARMOR, armorPkt);
		});
}

void NetSendSystem::SendAmmoEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvAmmoChanged>([&](const EvAmmoChanged& e)
		{
			if (!e.target.IsValid())
				return;
			
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.target);
			if (netComp == nullptr || netComp->mSessionId == 0)
				return;

			S2C_AmmoPacket ammoPkt;
			ammoPkt.netEntityId = netComp->mNetEntityId;
			ammoPkt.currentAmmo = e.currentAmmo;
			ammoPkt.maxAmmo = e.maxAmmo;
			cout << "send ammo : " << "ammoPkt.currentAmmo" << endl;
			mSendReq.SessionId = netComp->mSessionId;
			mSendReq.Type = S2C_PKT_AMMO;
			mSendReq.Size = sizeof(S2C_AmmoPacket);
			mSendReq.StoreAs<S2C_AmmoPacket>(ammoPkt);
			gSendQueue.Push(mSendReq);
		});
}


void NetSendSystem::SendBulletDeactivateEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	eventManager->Consume<EvBulletDeactivated>([&](const EvBulletDeactivated& e)
		{
			if (!e.bullet.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.bullet);
			if (netComp == nullptr)
				return;

			BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(e.bullet);
			if (bulletComp == nullptr)
				return;

			S2C_BulletDeactivatePacket deactivatePkt;
			deactivatePkt.bulletNetEntityId = netComp->mNetEntityId;
			deactivatePkt.bulletGeneration = bulletComp->mGeneration;

			Broadcast(recipients, S2C_PKT_BULLET_DEACTIVATE, deactivatePkt);
		});
}

void NetSendSystem::SendHitConfirmEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvHitConfirm>([&](const EvHitConfirm& e)
		{
			if (!e.instigator.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.instigator);
			if (netComp == nullptr || netComp->mSessionId == 0)
				return;

			NetEntityComponent* victimNet = mWorld->GetComponent<NetEntityComponent>(e.target);

			S2C_HitConfirmPacket pkt;
			pkt.victimNetEntityId = victimNet ? victimNet->mNetEntityId : 0;
			pkt.damage = e.damage;
			pkt.isKill = e.isKill ? 1 : 0;

			mSendReq.SessionId = netComp->mSessionId;
			mSendReq.Type = S2C_PKT_HIT_CONFIRM;
			mSendReq.Size = sizeof(S2C_HitConfirmPacket);
			mSendReq.StoreAs<S2C_HitConfirmPacket>(pkt);
			gSendQueue.Push(mSendReq);
		});
}

void NetSendSystem::SendEffectSpawnEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	eventManager->Consume<EvEffectSpawn>([&](const EvEffectSpawn& e)
		{
			S2C_EffectSpawnPacket effectPkt;
			effectPkt.effectType = e.effectType;
			effectPkt.x = e.x;
			effectPkt.y = e.y;
			effectPkt.z = e.z;
			effectPkt.reason = static_cast<uint8>(e.reason);
			effectPkt.rotX = e.rotX;
			effectPkt.rotY = e.rotY;
			effectPkt.rotZ = e.rotZ;

			//cout << "eff:" << (int)e.effectType << "   " << (int)effectPkt.reason << endl;

			Broadcast(recipients, S2C_PKT_EFFECT_SPAWN, effectPkt);
		});
}


// ─── 신규 세션 입장 처리 ─────────────────────────────────────

void NetSendSystem::HandleSessionJoinedEvents()
{
	std::shared_ptr<EventManager>& eventManager = mWorld->GetEventManager();
	if (eventManager == nullptr)
		return;

	eventManager->Consume<EvSessionJoined>([&](const EvSessionJoined& e)
	{
		SendPlayerSelfSpawn(e.sessionId, e.playerEntity, e.playerType);
		BroadcastPlayerToOthers(e.sessionId, e.playerEntity, e.playerType);
		SendExistingPlayersToNewSession(e.sessionId);
		SendWorldObjectsToNewSession(e.sessionId);
		SendEnemyPoolToNewSession(e.sessionId);
		SendBulletPoolToNewSession(e.sessionId);
		SendHealthSnapshotToNewSession(e.sessionId);
		SendArmorSnapshotToNewSession(e.sessionId);
	});
}

void NetSendSystem::SendPlayerSelfSpawn(uint32 sessionId, Entity playerEntity, uint8 playerType)
{
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (netComp == nullptr) return;

	S2C_SpawnPacekt spawnPkt(sessionId, netComp->mNetEntityId, PrefabType::PLAYER);
	spawnPkt.Type = playerType;
	spawnPkt.isLocalPlayer = 1;

	SendRequest req{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
	req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
	gSendQueue.Push(req);
}

void NetSendSystem::BroadcastPlayerToOthers(uint32 sessionId, Entity playerEntity, uint8 playerType)
{
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (netComp == nullptr) return;

	S2C_SpawnPacekt spawnPkt(sessionId, netComp->mNetEntityId, PrefabType::PLAYER);
	spawnPkt.Type = playerType;
	spawnPkt.isLocalPlayer = 0;

	for (uint32 otherSessionId : CollectPlayerSessions())
	{
		if (otherSessionId == sessionId) continue;

		SendRequest req{ otherSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::DespawnPlayerBySession(uint32 sessionId)
{
	if (!mWorld->HasComponentPool<NetEntityComponent>() || !mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	// 세션에 플레이어 엔티티 탐색
	bool found = false;
	Entity target;
	uint64 netId = 0;
	for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId == sessionId)
		{
			target = entity;
			netId = netComp->mNetEntityId;
			found = true;
			break;
		}
	}
	if (!found)
		return;

	// 본인을 제외한 같은 World 의 다른 플레이어들에게 despawn 통지
	S2C_DespawnPacket despawnPkt(netId);
	for (uint32 otherSessionId : CollectPlayerSessions())
	{
		if (otherSessionId == sessionId) continue;

		SendRequest req{ otherSessionId, PKT_Type::S2C_PKT_DESPAWN, sizeof(S2C_DespawnPacket) };
		req.StoreAs<S2C_DespawnPacket>(despawnPkt);
		gSendQueue.Push(req);
	}

	// 서버 World 에서 엔티티 파괴
	mWorld->DestroyEntity(target);
}

void NetSendSystem::SendExistingPlayersToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<NetEntityComponent>() || !mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);
		if (netComp == nullptr) continue;
		if (netComp->mSessionId == newSessionId || netComp->mSessionId == 0) continue;

		S2C_SpawnPacekt spawnPkt(netComp->mSessionId, netComp->mNetEntityId, PrefabType::PLAYER);
		spawnPkt.Type = playerComp ? playerComp->mPlayerType : 1;

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::SendWorldObjectsToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<NetEntityComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponent<NetEntityComponent>())
	{
		if (!entity.IsValid())
			continue;
		if (mWorld->HasComponent<MainPlayerComponent>(entity) ||
			mWorld->HasComponent<EnemyComponent>(entity) ||
			mWorld->HasComponent<BulletComponent>(entity))
		{
			continue;
		}

		PrefabType prefabType = PrefabType::NONE;

		if (mWorld->HasComponent<TruckComponent>(entity))
		{
			prefabType = PrefabType::TRUCK;
		}
		else if (InteractableComponent* interactable = mWorld->GetComponent<InteractableComponent>(entity))
		{
			if (!interactable->mActive)
				continue;

			switch (interactable->mKind)
			{
			case InteractableKind::HealPack:
				prefabType = PrefabType::HEAL_PACK;
				break;
			case InteractableKind::JumpPad:
				prefabType = PrefabType::JUMP_PAD;
				break;
			default:
				continue;
			}
		}
		else if (SpawnerComponent* spawner = mWorld->GetComponent<SpawnerComponent>(entity))
		{
			if (!spawner->mActive)
				continue;
			prefabType = PrefabType::MONSTER_SPAWNER;
		}
		else
		{
			continue;
		}

		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr)
			continue;

		S2C_SpawnPacekt spawnPkt(newSessionId, netComp->mNetEntityId, prefabType);
		spawnPkt.isLocalPlayer = 0;
		if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity))
		{
			spawnPkt.hasInitialTransform = 1;
			spawnPkt.x = transform->mLocalPosition.x;
			spawnPkt.y = transform->mLocalPosition.y;
			spawnPkt.z = transform->mLocalPosition.z;
		}

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::SendEnemyPoolToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<EnemyComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, EnemyComponent>())
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(entity);
		if (netComp == nullptr || enemyComp == nullptr) continue;

		S2C_SpawnPacekt spawnPkt(newSessionId, netComp->mNetEntityId, PrefabType::ENEMY);
		spawnPkt.isLocalPlayer = 0;
		spawnPkt.Type = enemyComp->mEnemyType;

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::SendBulletPoolToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<BulletComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, BulletComponent>())
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;

		S2C_SpawnPacekt spawnPkt(newSessionId, netComp->mNetEntityId, PrefabType::BULLET);
		spawnPkt.isLocalPlayer = 0;

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		req.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::SendHealthSnapshotToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<HealthComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponents<HealthComponent, NetEntityComponent>())
	{
		HealthComponent* hp = mWorld->GetComponent<HealthComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (hp == nullptr || netComp == nullptr) continue;

		S2C_HealthPacket healthPkt;
		healthPkt.netEntityId = netComp->mNetEntityId;
		healthPkt.currentHp = hp->mCurrentHp;
		healthPkt.maxHp = hp->mMaxHp;

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_HEALTH, sizeof(S2C_HealthPacket) };
		req.StoreAs<S2C_HealthPacket>(healthPkt);
		gSendQueue.Push(req);
	}
}

void NetSendSystem::SendArmorSnapshotToNewSession(uint32 newSessionId)
{
	if (!mWorld->HasComponentPool<ArmorComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return;

	for (auto entity : mWorld->GetEntitiesWithComponents<ArmorComponent, NetEntityComponent>())
	{
		ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (armor == nullptr || netComp == nullptr) continue;

		S2C_ArmorPacket armorPkt;
		armorPkt.netEntityId = netComp->mNetEntityId;
		armorPkt.currentArmor = armor->mCurrentArmor;
		armorPkt.maxArmor = armor->mMaxArmor;

		SendRequest req{ newSessionId, PKT_Type::S2C_PKT_ARMOR, sizeof(S2C_ArmorPacket) };
		req.StoreAs<S2C_ArmorPacket>(armorPkt);
		gSendQueue.Push(req);
	}
}
