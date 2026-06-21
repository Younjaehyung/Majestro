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
#include "BuffComponent.h"
#include "GameTimer.h"

namespace
{
	ReplicatedBuffType ToReplicatedBuffType(BuffType type)
	{
		switch (type)
		{
		case BuffType::AttackUp: return ReplicatedBuffType::AttackUp;
		case BuffType::CritAttackUp: return ReplicatedBuffType::AttackUp;
		case BuffType::ScoreBoost: return ReplicatedBuffType::ScoreBoost;
		case BuffType::MoveSpeedUp: return ReplicatedBuffType::MoveSpeedUp;
		case BuffType::CritMoveSpeedUp: return ReplicatedBuffType::MoveSpeedUp;
		case BuffType::BuffPowerUp: return ReplicatedBuffType::BuffPowerUp;
		case BuffType::MoveSpeedUp10: return ReplicatedBuffType::MoveSpeedUp10;
		case BuffType::ShieldOverTime: return ReplicatedBuffType::ShieldOverTime;
		case BuffType::HealOverTime: return ReplicatedBuffType::HealOverTime;
		case BuffType::ShieldDown: return ReplicatedBuffType::ShieldDown;
		case BuffType::Silence: return ReplicatedBuffType::Silence;
		default: return ReplicatedBuffType::None;
		}
	}
}

NetSendSystem::NetSendSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

void NetSendSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	// 실시간 서버 클럭 누적
	mServerClockSec += dt;


	HandleSessionJoinedEvents();   // 신규 세션 초기 상태 송신
	SendAction();
	if (mPlayerStatusRate.Tick(dt))
		SendPlayerStatus();
	SendCollision();
	SendHealthEvents();
	SendArmorEvents();
	SendAmmoEvents();
	SendCooldownEvents();
	SendBulletDeactivateEvents();
	SendEffectSpawnEvents();
	SendHitConfirmEvents();
	SendGimmickStateEvents();
	SendRhythmChangedEvents();


	if (mMovementRate.Tick(dt))           // 30Hz 주기 전송 (UDP)
		SendMove(dt);	//move


}

void NetSendSystem::SendPlayerStatus()
{
	if (!mWorld->HasComponentPool<MainPlayerComponent>() ||
		!mWorld->HasComponentPool<NetEntityComponent>())
	{
		return;
	}

	const float now = GetServerTotalTimeSeconds();
	for (Entity entity : mWorld->GetEntitiesWithComponents<MainPlayerComponent, NetEntityComponent>())
	{
		MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
		NetEntityComponent* net = mWorld->GetComponent<NetEntityComponent>(entity);
		if (!player || !net || net->mSessionId == 0)
			continue;

		S2C_PlayerStatusPacket packet{};
		packet.netEntityId = net->mNetEntityId;

		if (player->IsDeathActive())
		{
			packet.statusFlags |= PlayerStatus_Dead;
			packet.respawnRemaining = max(0.0f, player->mDeathEndTime - now);
		}

		if (player->GetReplicatedActionState() ==
			static_cast<uint8>(ReplicatedActionState::Stun))
		{
			packet.statusFlags |= PlayerStatus_Stunned;
			packet.stunRemaining = max(0.0f, player->mStateEnd - now);
		}

		if (BuffComponent* buffs = mWorld->GetComponent<BuffComponent>(entity))
		{
			for (const BuffData& buff : buffs->mBuffs)
			{
				if (packet.buffCount >= MAX_REPLICATED_BUFFS)
					break;

				const ReplicatedBuffType replicatedType =
					ToReplicatedBuffType(buff.mType);
				if (replicatedType == ReplicatedBuffType::None)
					continue;

				ReplicatedBuffState& state = packet.buffs[packet.buffCount++];
				state.buffType = replicatedType;
				state.buffFlags = ReplicatedBuff_None;

				if (buff.mDurationPolicy == DurationPolicy::Timed)
				{
					state.buffFlags |= ReplicatedBuff_Timed;
					state.remainingTime = max(0.0f, buff.mEndTime - now);
				}
				else
				{
					state.remainingTime = -1.0f;
				}

				if (buff.mExecutionType == BuffExecutionType::Periodic)
					state.buffFlags |= ReplicatedBuff_Periodic;
				if (buff.mKind == EffectKind::Debuff)
					state.buffFlags |= ReplicatedBuff_Debuff;
				if (buff.mIsRhythmEffect)
					state.buffFlags |= ReplicatedBuff_Rhythm;
			}
		}

		SendRequest request{};
		request.SessionId = net->mSessionId;
		request.Type = S2C_PKT_PLAYER_STATUS;
		request.StoreAs(packet);
		gSendQueue.Push(request);
	}
}

void NetSendSystem::SendMove(float dt)
{

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	const uint32 sendTick = static_cast<uint32>(mServerClockSec * 60.0);

	// mMovingVector 초당 속도
	constexpr float kServerSimHz = 60.0f;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		if (mWorld->HasComponent<BulletComponent>(entity))
			continue;
		{

			TransformComponent* transComp = mWorld->GetComponent<TransformComponent>(netComp->mOwnerEntity);

			// 정적 엔티티는 이동 송신에서 제외
			if (transComp == nullptr || transComp->mIsStatic)
				continue;

			S2C_MovePacket movePkt;

			movePkt.netEntityId = netComp->mNetEntityId;
			movePkt.Sequence = sendTick;
			movePkt.x = transComp->mWorldPosition.x;
			movePkt.y = transComp->mWorldPosition.y;
			movePkt.z = transComp->mWorldPosition.z;

			movePkt.vx = transComp->mMovingVector.x * kServerSimHz;
			movePkt.vy = transComp->mMovingVector.y * kServerSimHz;
			movePkt.vz = transComp->mMovingVector.z * kServerSimHz;
			
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
			healthPkt.isCritical = e.isCritical ? 1 : 0;

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

	auto sendAmmoPacket = [&](Entity target, int32 currentAmmo, int32 maxAmmo)
		{
			if (!target.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(target);
			MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(target);
			if (netComp == nullptr || netComp->mSessionId == 0 || playerComp == nullptr)
				return;

			S2C_AmmoPacket ammoPkt;
			ammoPkt.netEntityId = netComp->mNetEntityId;
			ammoPkt.currentAmmo = currentAmmo;
			ammoPkt.maxAmmo = maxAmmo;

			mSendReq.SessionId = netComp->mSessionId;
			mSendReq.Type = S2C_PKT_AMMO;
			mSendReq.Size = sizeof(S2C_AmmoPacket);
			mSendReq.StoreAs<S2C_AmmoPacket>(ammoPkt);
			gSendQueue.Push(mSendReq);

			playerComp->mLastReplicatedAmmo = currentAmmo;
		};

	eventManager->Consume<EvAmmoChanged>([&](const EvAmmoChanged& e)
		{
			sendAmmoPacket(e.target, e.currentAmmo, e.maxAmmo);
		});

	if (!mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	auto players = mWorld->GetEntitiesWithComponents<MainPlayerComponent, NetEntityComponent>();
	for (Entity player : players)
	{
		MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(player);
		if (playerComp == nullptr)
			continue;

		if (playerComp->mLastReplicatedAmmo == playerComp->mNowBullet)
			continue;

		sendAmmoPacket(player, static_cast<int32>(playerComp->mNowBullet), static_cast<int32>(playerComp->mMaxBullet));
	}
}


void NetSendSystem::SendCooldownEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvCooldownStarted>([&](const EvCooldownStarted& e)
		{
			if (!e.target.IsValid())
				return;

			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.target);
			if (netComp == nullptr || netComp->mSessionId == 0)
				return;

			S2C_CooldownPacket pkt(netComp->mNetEntityId, e.skillSlot, e.durationSeconds, e.durationSeconds);
			mSendReq.SessionId = netComp->mSessionId;
			mSendReq.Type = S2C_PKT_COOLDOWN;
			mSendReq.Size = sizeof(S2C_CooldownPacket);
			mSendReq.StoreAs<S2C_CooldownPacket>(pkt);
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
			effectPkt.casterNetId = e.casterNetId;

			//cout << "eff:" << (int)e.effectType << "   " << (int)effectPkt.reason << endl;

			Broadcast(recipients, S2C_PKT_EFFECT_SPAWN, effectPkt);
		});
}

void NetSendSystem::SendGimmickStateEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	// 힐팩 등의 숨김/재등장을 전체 세션에 통지
	eventManager->Consume<EvInteractableStateChanged>([&](const EvInteractableStateChanged& e)
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.trigger);
			if (netComp == nullptr)
				return;

			S2C_GimmickStatePacket pkt(netComp->mNetEntityId, e.active);
			Broadcast(recipients, S2C_PKT_GIMMICK_STATE, pkt);
		});
}

void NetSendSystem::SendRhythmChangedEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	eventManager->Consume<EvRhythmChanged>([&](const EvRhythmChanged& e)
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.player);
			if (netComp == nullptr)
				return;

			S2C_RhythmChangedPacket pkt{};
			pkt.netEntityId = netComp->mNetEntityId;
			pkt.applyAtBeatIndex = e.applyAtBeatIndex;
			pkt.previousRhythm = e.previousRhythm;
			pkt.changedRhythm = e.changedRhythm;
			pkt.playerType = e.playerType;

			Broadcast(recipients, S2C_PKT_RHYTHM_CHANGED, pkt);
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
	if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(playerEntity))
	{
		// PreparePhase에서 확정한 캐릭터별 위치를 클라이언트 최초 생성 좌표로 전달한다.
		spawnPkt.hasInitialTransform = 1;
		spawnPkt.x = transform->mLocalPosition.x;
		spawnPkt.y = transform->mLocalPosition.y;
		spawnPkt.z = transform->mLocalPosition.z;
	}

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
	if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(playerEntity))
	{
		// 다른 클라이언트에도 같은 캐릭터별 스폰 위치를 전달한다.
		spawnPkt.hasInitialTransform = 1;
		spawnPkt.x = transform->mLocalPosition.x;
		spawnPkt.y = transform->mLocalPosition.y;
		spawnPkt.z = transform->mLocalPosition.z;
	}

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
		if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity))
		{
			// 늦게 입장한 클라이언트도 기존 플레이어의 현재 위치에서 생성한다.
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
		bool hiddenForClients = false; // 쿨타임 중 숨김 상태면 스폰 직후 숨김 통지를 같이 보냄

		if (mWorld->HasComponent<TruckComponent>(entity))
		{
			prefabType = PrefabType::TRUCK;
		}
		else if (InteractableComponent* interactable = mWorld->GetComponent<InteractableComponent>(entity))
		{
			if (!interactable->mActive)
				continue;

			hiddenForClients = interactable->mHiddenForClients;

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
			prefabType = PrefabType::MONSTER_SPAWNER;
			// 비활성 스포너도 마커는 미리 보내두고, 숨김 상태로 시작
			hiddenForClients = !spawner->mActive;
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

		// 쿨타임 중인 픽업은 스폰 후 바로 숨김 상태
		if (hiddenForClients)
		{
			S2C_GimmickStatePacket statePkt(netComp->mNetEntityId, false);
			SendRequest stateReq{ newSessionId, PKT_Type::S2C_PKT_GIMMICK_STATE, sizeof(S2C_GimmickStatePacket) };
			stateReq.StoreAs<S2C_GimmickStatePacket>(statePkt);
			gSendQueue.Push(stateReq);
		}
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

		if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity))
		{
			// 기존 적 스폰 패킷에도 현재 위치를 포함해 클라이언트의 원점 생성을 방지한다
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
