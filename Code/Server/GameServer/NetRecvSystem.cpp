#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "MovementComponent.h"
#include "GameEvents.h"
#include "EventManager.h"

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

	inputComp->AimCameraPosition  = Vec3(pkt.CameraX, pkt.CameraY, pkt.CameraZ);
	inputComp->AimCameraDirection = Vec3(pkt.CameraDirX, pkt.CameraDirY, pkt.CameraDirZ);
	inputComp->HasAimCameraRay    = inputComp->AimCameraDirection.LengthSquared() > 0.0001f;
	if (inputComp->HasAimCameraRay)
		inputComp->AimCameraDirection.Normalize();
}

void NetRecvSystem::RecvAction(uint32 sessionId, const C2S_ActionPacket& pkt)
{
	Entity e = FindEntityBySession(sessionId);
	if (!e.IsValid()) return;

	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
	if (inputComp == nullptr) return;

	inputComp->Buttons = pkt.Buttons;
	
	inputComp->Yaw = pkt.Yaw;
	inputComp->Pitch = pkt.Pitch;
	inputComp->AimCameraPosition = Vec3(pkt.CameraX, pkt.CameraY, pkt.CameraZ);
	inputComp->AimCameraDirection = Vec3(pkt.CameraDirX, pkt.CameraDirY, pkt.CameraDirZ);
	inputComp->HasAimCameraRay = inputComp->AimCameraDirection.LengthSquared() > 0.0001f;
	if (inputComp->HasAimCameraRay)
		inputComp->AimCameraDirection.Normalize();
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
	Entity playerEntity = SpawnPlayer(inputCommand);
	if (!playerEntity.IsValid())
		return;

	EnsureEnemyPool(inputCommand);
	EnsureBulletPool(inputCommand);

	uint8 playerType = 1;
	if (MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(playerEntity))
		playerType = playerComp->mPlayerType;

	if (std::shared_ptr<EventManager>& eventManager = mWorld->GetEventManager())
	{
		EvSessionJoined ev{};
		ev.sessionId = inputCommand.SessionId;
		ev.playerEntity = playerEntity;
		ev.playerType = playerType;
		eventManager->Enqueue<EvSessionJoined>(ev);
	}
}

// ─── 플레이어 스폰 ───────────────────────────────────────────

Entity NetRecvSystem::SpawnPlayer(InputCommand& inputCommand)
{
	// 중복 패킷 방어: 이미 같은 세션의 플레이어가 존재하면 무시
	if (mWorld->HasComponentPool<NetEntityComponent>() &&
		mWorld->HasComponentPool<MainPlayerComponent>())
	{
		for (auto entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			if (netComp && netComp->mSessionId == inputCommand.SessionId)
				return Entity{};
		}
	}

	const C2S_StartGamePacket* startPacket = inputCommand.ViewAs<C2S_StartGamePacket>();
	if (startPacket)
	{
		cout << "charactor: " << (int)startPacket->playerType << endl;
	}

	return PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, inputCommand);
}

// ─── 에너미 / 불릿 풀 스폰 ──────────────────────────────────

void NetRecvSystem::EnsureEnemyPool(InputCommand& inputCommand)
{
	if (!mEnemySpawnOnce)
		return;

	for (int i = 0; i < 10; ++i)
	{
		PrefabFactory::Spawn(mWorld, PrefabType::ENEMY, inputCommand);
	}
	mEnemySpawnOnce = false;
}

void NetRecvSystem::EnsureBulletPool(InputCommand& inputCommand)
{
	if (!mBulletSpawnOnce)
		return;

	constexpr int kBulletPoolSize = 64;
	for (int i = 0; i < kBulletPoolSize; ++i)
	{
		PrefabFactory::Spawn(mWorld, PrefabType::BULLET, inputCommand);
	}
	mBulletSpawnOnce = false;
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
