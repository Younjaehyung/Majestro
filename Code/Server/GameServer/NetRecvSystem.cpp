#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "HealthComponent.h"
#include "MovementComponent.h"
#include "BulletComponent.h"
#include "GameEvents.h"
#include "EventManager.h"
#include "InteractableComponent.h"
#include "SpawnerComponent.h"
#include "EnemyComponent.h"

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
	mPhase = SysPhase::Pre;
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;

	while (processed < kMaxMsgsPerTick && mWorld->DequeueCommand(mInputCommand))
	{
		switch (mInputCommand.Type)
		{
			case PKT_Type::C2S_PKT_MOVE:
			{
				const C2S_MovePacket* pkt = mInputCommand.ViewAs<C2S_MovePacket>();
				if (pkt) RecvInput(mInputCommand.SessionId, *pkt);
				break;
			}
			case PKT_Type::C2S_PKT_ACTION:
			{
				const C2S_ActionPacket* pkt = mInputCommand.ViewAs<C2S_ActionPacket>();
				if (pkt) RecvAction(mInputCommand.SessionId, *pkt);
				break;
			}
			case PKT_Type::C2S_PKT_RHYTHM_CHANGED:
			{
				const C2S_RhythmChangedPacket* pkt = mInputCommand.ViewAs<C2S_RhythmChangedPacket>();
				if (pkt) RecvRhythmChanged(mInputCommand.SessionId, *pkt);
				break;
			}
			case PKT_Type::C2S_GAME_START:
			{
				HandleGameStart(mInputCommand);
				break;
			}
		}
		++processed;
	}
}

// ─── 입력 수신 ────────────────────────────────────────────────

void NetRecvSystem::RecvInput(uint32 sessionId, const C2S_MovePacket& pkt)
{
	Entity e = FindEntityBySession(sessionId);
	if (!e.IsValid()) return;

	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
	if (inputComp == nullptr) return;

	// 최신 입력만 반영 (out-of-order/중복 드롭)
	if (inputComp->lastSeq != 0)
	{
		if (!IsNewerSeq(pkt.Seq, inputComp->lastSeq))
			return;
	}

	inputComp->MoveX  = pkt.MoveX;
	inputComp->MoveY  = pkt.MoveY;
	inputComp->MoveZ  = pkt.MoveZ;
	inputComp->Yaw    = pkt.Yaw;
	inputComp->Pitch  = pkt.Pitch;
	inputComp->lastSeq = pkt.Seq;
}

void NetRecvSystem::RecvAction(uint32 sessionId, const C2S_ActionPacket& pkt)
{
	Entity e = FindEntityBySession(sessionId);
	if (!e.IsValid()) return;

	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
	if (inputComp == nullptr) return;

	inputComp->Buttons = pkt.Buttons;
}

void NetRecvSystem::RecvRhythmChanged(uint32 sessionId, const C2S_RhythmChangedPacket& pkt)
{
	Entity e = FindEntityBySession(sessionId);
	if (!e.IsValid())
		return;

	MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
	if (playerComp == nullptr)
		return;

	if (pkt.netEntityId != 0)
	{
		NetEntityComponent* netEntityComp = mWorld->GetComponent<NetEntityComponent>(e);
		if (netEntityComp && netEntityComp->mNetEntityId != pkt.netEntityId)
			return;
	}

	const uint8 previousRhythm = static_cast<uint8>(pkt.previousRhythm % 4);
	const uint8 changedRhythm = static_cast<uint8>(pkt.changedRhythm % 4);
	if (previousRhythm != playerComp->mRhythm)
		return;

	if (changedRhythm == playerComp->mRhythm)
		return;

	playerComp->mRhythm = previousRhythm;
	playerComp->mNextRhythm = changedRhythm;
	playerComp->mHasQueuedRhythmChange = true;
}

// ─── 게임 시작 처리 ──────────────────────────────────────────

void NetRecvSystem::HandleGameStart(InputCommand& inputCommand)
{
	SpawnPlayer(inputCommand);
	EnemySpawnProcess(inputCommand);
	BulletPoolSpawnProcess(inputCommand);
	SendWorldObjectsToNewClient(inputCommand.SessionId);
}

// ─── 플레이어 스폰 ───────────────────────────────────────────

void NetRecvSystem::SpawnPlayer(InputCommand& inputCommand)
{
	// 중복 패킷 방어: 이미 같은 세션의 플레이어가 존재하면 무시
	if (mWorld->HasComponentPool<NetEntityComponent>() &&
		mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			if (netComp && netComp->mSessionId == inputCommand.SessionId)
				return;
		}
	}

	uint8 playerType = 1;
	const C2S_StartGamePacket* startPacket = inputCommand.ViewAs<C2S_StartGamePacket>();
	if (startPacket)
	{
		playerType = startPacket->playerType;
		cout << "charactor: " << (int)startPacket->playerType << endl;
	}
	if (playerType < 1 || playerType > 3)
		playerType = 1;

	Entity e = PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, inputCommand);

	MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
	if (playerComp)
		playerType = playerComp->mPlayerType;

	SendSpawnToSelf(inputCommand.SessionId, e, playerType);
	BroadcastSpawnToOthers(inputCommand.SessionId, e, playerType);
	SendExistingPlayersToNewClient(inputCommand.SessionId);
}

void NetRecvSystem::SendSpawnToSelf(uint32 sessionId, Entity e, uint8 playerType)
{
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr) return;

	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(sessionId, netComp->mNetEntityId, PrefabType::PLAYER);
	spawnPkt.Type = playerType;
	spawnPkt.isLocalPlayer = 1;

	SendRequest request{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
	request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
	gSendQueue.Push(request);
}

void NetRecvSystem::BroadcastSpawnToOthers(uint32 sessionId, Entity e, uint8 playerType)
{
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr) return;

	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(sessionId, netComp->mNetEntityId, PrefabType::PLAYER);
	spawnPkt.Type = playerType;
	spawnPkt.isLocalPlayer = 0;

	for (uint32 otherSessionId : CollectPlayerSessions())
	{
		if (otherSessionId == sessionId) continue;

		SendRequest request{ otherSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}
}

void NetRecvSystem::SendExistingPlayersToNewClient(uint32 newSessionId)
{
	for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
	{
		NetEntityComponent* netComp  = mWorld->GetComponent<NetEntityComponent>(entity);
		MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);
		if (netComp == nullptr) continue;
		if (netComp->mSessionId == newSessionId || netComp->mSessionId == 0) continue;

		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(netComp->mSessionId, netComp->mNetEntityId, PrefabType::PLAYER);
		spawnPkt.Type = playerComp ? playerComp->mPlayerType : 1;

		SendRequest request{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}
}

void NetRecvSystem::SendWorldObjectsToNewClient(uint32 newSessionId)
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
		if (InteractableComponent* interactable = mWorld->GetComponent<InteractableComponent>(entity))
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

		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(newSessionId, netComp->mNetEntityId, prefabType);
		spawnPkt.isLocalPlayer = 0;

		SendRequest request{ newSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}
}

// ─── 에너미 / 불릿 풀 스폰 ──────────────────────────────────

void NetRecvSystem::EnemySpawnProcess(InputCommand& inputCommand)
{
	if (mEnemySpawnOnce)
	{
		mEnemySpawnInfos.reserve(200);
		for (int i = 0; i < 10; ++i)
		{
			Entity e = PrefabFactory::Spawn(mWorld, PrefabType::ENEMY, inputCommand);
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
			EnemyComponent* emyComp = mWorld->GetComponent<EnemyComponent>(e);
			mEnemySpawnInfos.emplace_back(netComp->mNetEntityId, emyComp->mEnemyType);
		}
		mEnemySpawnOnce = false;
	}

	for (auto [id, enemyType] : mEnemySpawnInfos)
	{
		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(inputCommand.SessionId, id, PrefabType::ENEMY);
		spawnPkt.isLocalPlayer = 0;
		spawnPkt.Type = enemyType;

		for (uint32 sessionId : CollectPlayerSessions())
		{
			SendRequest request{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
			gSendQueue.Push(request);
		}
	}

	auto eventManager = mWorld->GetEventManager();
	if (eventManager == nullptr) return;

	for (auto enemy : mWorld->GetEntitiesWithComponents<NetEntityComponent, EnemyMovementComponent, HealthComponent>())
	{
		HealthComponent* healthComp = mWorld->GetComponent<HealthComponent>(enemy);
		if (healthComp == nullptr) continue;

		EvHealthChanged healthChanged{};
		healthChanged.target    = enemy;
		healthChanged.currentHp = healthComp->mCurrentHp;
		healthChanged.maxHp     = healthComp->mMaxHp;
		eventManager->Enqueue<EvHealthChanged>(healthChanged);
	}
}

void NetRecvSystem::BulletPoolSpawnProcess(InputCommand& inputCommand)
{
	if (mBulletSpawnOnce)
	{
		constexpr int kBulletPoolSize = 64;
		mBulletNetEntityIds.reserve(kBulletPoolSize);
		for (int i = 0; i < kBulletPoolSize; ++i)
		{
			Entity bulletEntity = PrefabFactory::Spawn(mWorld, PrefabType::BULLET, inputCommand);
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(bulletEntity);
			if (netComp != nullptr)
				mBulletNetEntityIds.push_back(netComp->mNetEntityId);
		}
		mBulletSpawnOnce = false;
	}

	for (uint64 bulletNetId : mBulletNetEntityIds)
	{
		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(inputCommand.SessionId, bulletNetId, PrefabType::BULLET);
		spawnPkt.isLocalPlayer = 0;

		SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}
}

// ─── 유틸 ────────────────────────────────────────────────────

Entity NetRecvSystem::FindEntityBySession(uint32 sessionId) const
{
	if (!mWorld->HasComponentPool<InputComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return Entity{};

	for (auto entity : mWorld->GetEntitiesWithComponent<InputComponent>())
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId == sessionId)
			return entity;
	}
	return Entity{};
}

std::vector<uint32> NetRecvSystem::CollectPlayerSessions() const
{
	if (!mWorld->HasComponentPool<NetEntityComponent>())
		return {};

	std::unordered_set<uint32> sessionSet;
	auto entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	sessionSet.reserve(entities.size());

	for (auto entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId != 0)
			sessionSet.insert(netComp->mSessionId);
	}

	return std::vector<uint32>(sessionSet.begin(), sessionSet.end());
}
