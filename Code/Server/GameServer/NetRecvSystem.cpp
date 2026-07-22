#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "ChatComponent.h"
#include "InputComponent.h"
#include "EmoteComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "MovementComponent.h"
#include "GameEvents.h"
#include "EventManager.h"
#include "GamePhase.h"
#include "BeatSystem.h"
#include "GameTimer.h"
#include "Protocol/RhythmDefinitions.h"
#include "RhythmComponents.h"
#include "TransformComponent.h"
#include "GravityComponent.h"
#include "FlyComponent.h"
#include "HealthComponent.h"
#include "EnemyComponent.h"

namespace
{
	void BroadcastEnemySpawn(World* world, Entity spawnedEntity)
	{
		if (!world || !spawnedEntity.IsValid())
			return;

		NetEntityComponent* netComp = world->GetComponent<NetEntityComponent>(spawnedEntity);
		EnemyComponent* enemyComp = world->GetComponent<EnemyComponent>(spawnedEntity);
		if (!netComp || !enemyComp)
			return;

		S2C_SpawnPacekt spawnPkt(0, netComp->mNetEntityId, PrefabType::ENEMY);
		spawnPkt.isLocalPlayer = 0;
		spawnPkt.Type = enemyComp->mEnemyType;

		if (TransformComponent* transform = world->GetComponent<TransformComponent>(spawnedEntity))
		{
			spawnPkt.hasInitialTransform = 1;
			spawnPkt.x = transform->mLocalPosition.x;
			spawnPkt.y = transform->mLocalPosition.y;
			spawnPkt.z = transform->mLocalPosition.z;
		}

		for (auto playerEntity : world->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
		{
			NetEntityComponent* playerNetComp = world->GetComponent<NetEntityComponent>(playerEntity);
			if (!playerNetComp || playerNetComp->mSessionId == 0)
				continue;

			SendRequest request{ playerNetComp->mSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
			gSendQueue.Push(request);
		}

		if (auto eventManager = world->GetEventManager())
		{
			if (HealthComponent* hp = world->GetComponent<HealthComponent>(spawnedEntity))
			{
				EvHealthChanged evt{};
				evt.target = spawnedEntity;
				evt.currentHp = hp->mCurrentHp;
				evt.maxHp = hp->mMaxHp;
				eventManager->Enqueue<EvHealthChanged>(evt);
			}
		}
	}
}

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
	mPhase = SysPhase::Pre;
	RegisterHandlers();
}

void NetRecvSystem::RegisterHandlers()
{
	auto reg = [&](PKT_Type type, Handler h) {
		mHandlers[static_cast<size_t>(type)] = std::move(h);
	};

	reg(PKT_Type::C2S_PKT_MOVE, [this](InputCommand& c) {
		if (const C2S_MovePacket* pkt = c.ViewAs<C2S_MovePacket>())
			RecvInput(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_PKT_ACTION, [this](InputCommand& c) {
		if (const C2S_ActionPacket* pkt = c.ViewAs<C2S_ActionPacket>())
			RecvAction(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_PKT_RHYTHM_CHANGED, [this](InputCommand& c) {
		if (const C2S_RhythmChangedPacket* pkt = c.ViewAs<C2S_RhythmChangedPacket>())
			RecvRhythmChanged(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_PKT_STICKER, [this](InputCommand& c) {
		// 이벤트로 전체 브로드캐스트
		if (const C2S_StickerPacket* pkt = c.ViewAs<C2S_StickerPacket>())
			mWorld->GetEventManager()->Enqueue<EvStickerBroadcast>(EvStickerBroadcast{
				pkt->casterNetId, pkt->camX, pkt->camY, pkt->camZ,
				pkt->dirX, pkt->dirY, pkt->dirZ, pkt->size, pkt->textureId });
	});
	reg(PKT_Type::C2S_PKT_EMOTE, [this](InputCommand& c) {
		if (const C2S_EmotePacket* pkt = c.ViewAs<C2S_EmotePacket>())
			RecvEmote(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_PKT_CHAT, [this](InputCommand& c) {
		if (const C2S_ChatPacket* pkt = c.ViewAs<C2S_ChatPacket>())
			RecvChat(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_PKT_SYNC, [this](InputCommand& c) {
		if (const C2S_SyncPacket* pkt = c.ViewAs<C2S_SyncPacket>())
			RecvSync(c.SessionId, *pkt);
	});
	reg(PKT_Type::C2S_GAME_START, [this](InputCommand& c) {
		HandleGameStart(c);
	});
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;

	while (processed < kMaxMsgsPerTick && mWorld->DequeueCommand(mInputCommand))
	{
		ProcessOne(mInputCommand);
		++processed;
	}
}

void NetRecvSystem::ProcessOne(InputCommand& cmd)
{
	const size_t idx = static_cast<size_t>(cmd.Type);
	if (idx < mHandlers.size() && mHandlers[idx])
		mHandlers[idx](cmd);
}

// ─── 입력 수신 ────────────────────────────────────────────────

void NetRecvSystem::RecvInput(uint32 sessionId, const C2S_MovePacket& pkt)
{
	Entity e = FindEntityBySession(sessionId);
	if (!e.IsValid()) return;

	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr || netComp->mNetEntityId != pkt.netEntityId)
	{
		// 이전 씬에서 늦게 도착한 UDP 입력이 새 씬 플레이어에게 적용되는 것을 막는다
		return;
	}

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
	inputComp->mLastMoveInputTime = GetServerTotalTimeSeconds();

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

	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr || netComp->mNetEntityId != pkt.netEntityId)
	{
		// 이전 씬의 지연된 액션 패킷이 새 씬 플레이어 상태를 변경하지 못하게 한다
		return;
	}

	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
	if (inputComp == nullptr) return;

	inputComp->Buttons = pkt.Buttons;
	inputComp->InputSongPos = pkt.inputSongPos; // 박자 판정 기준(입력 순간 곡 위치)

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
	RhythmStateComponent* rhythmState = mWorld->GetComponent<RhythmStateComponent>(e);

	if (playerComp == nullptr || rhythmState == nullptr)
		return;

	if (pkt.netEntityId != 0)
	{
		NetEntityComponent* netEntityComp = mWorld->GetComponent<NetEntityComponent>(e);
		if (netEntityComp && netEntityComp->mNetEntityId != pkt.netEntityId)
			return;
	}

	const Rhythm previousRhythm = SanitizeRhythm(pkt.previousRhythm);
	const Rhythm changedRhythm = SanitizeRhythm(pkt.changedRhythm);

	// 예약을 서버에 요청한 클라이언트에게 회신하는 람다
	auto enqueueRhythmReservation = [&](Rhythm targetRhythm, int64 applyAtBeatIndex)
	{
		if (std::shared_ptr<EventManager>& eventManager = mWorld->GetEventManager())
		{
			EvRhythmChanged ev{};
			ev.player = e;
			ev.previousRhythm = ToRhythmValue(rhythmState->GetCurrentRhythm());
			ev.changedRhythm = ToRhythmValue(targetRhythm);
			ev.playerType = playerComp->mPlayerType;
			ev.applyAtBeatIndex = applyAtBeatIndex;
			eventManager->Enqueue<EvRhythmChanged>(ev);
		}
	};

	// 현재 예약을 회신하는 람다
	auto enqueueCurrentReservation = [&]()
	{
		const Rhythm targetRhythm = rhythmState->HasPendingChange()
			? rhythmState->GetPendingRhythm()
			: rhythmState->GetCurrentRhythm();
		const int64 applyAtBeatIndex = rhythmState->HasPendingChange()
			? rhythmState->GetPendingApplyBeat()
			: kNoRhythmApplyBeat;
		enqueueRhythmReservation(targetRhythm, applyAtBeatIndex);
	};

	const uint8 movementMode = playerComp->GetReplicatedMovementMode();
	if (movementMode == static_cast<uint8>(ReplicatedMovementMode::Airborne) ||
		movementMode == static_cast<uint8>(ReplicatedMovementMode::Falling) ||
		movementMode == static_cast<uint8>(ReplicatedMovementMode::Landing))
	{
		// 요청이 거절되어도 현재 예약을 회신해 클라이언트의 응답 대기 끝냄
		enqueueCurrentReservation();
		return;
	}

	if (previousRhythm != rhythmState->GetCurrentRhythm())
	{
		// 적용 경계와 요청이 겹치면 서버의 현재 상태로 클라이언트를 다시 맞춤
		enqueueCurrentReservation();
		return;
	}

	// 검증된 우클릭 리듬 변경 요청은 서버 상태로 재생해 모든 클라이언트에 복제한다.
	playerComp->mFsm.ChangeState(playerComp, RhythmChangeState::Instance());

	if (changedRhythm == rhythmState->GetCurrentRhythm())
	{
		// 최종 선택이 현재 음악으로 돌아오면 기존 예약을 취소
		rhythmState->CancelPendingChange();
		enqueueRhythmReservation(changedRhythm, kNoRhythmApplyBeat);
		return;
	}

	if (rhythmState->HasPendingChange())
	{
		// 연타로 바뀐 최종 음악만 교체
		const int64 applyAtBeatIndex = rhythmState->GetPendingApplyBeat();
		rhythmState->ReplacePendingRhythm(changedRhythm);
		enqueueRhythmReservation(changedRhythm, applyAtBeatIndex);
		return;
	}

	BeatSystem* beatSystem = nullptr;

	if (auto systemManager = mWorld->GetSystemManager())
		beatSystem = systemManager->GetSystem<BeatSystem>();

	// 전환은 다음 음악 루프 경계에 예약
	int64 applyAtBeatIndex = kMusicLoopBeatCount;

	if (beatSystem)
	{
		const int64 cur = beatSystem->GetAbsoluteBeatIndex();

		int64 nextLoopBoundary =
			(cur / kMusicLoopBeatCount + 1) * kMusicLoopBeatCount;

		if (nextLoopBoundary - cur < kRhythmLookAheadBeats)
			nextLoopBoundary += kMusicLoopBeatCount;

		applyAtBeatIndex = nextLoopBoundary;
	}


	rhythmState->ScheduleRhythmChange(changedRhythm, applyAtBeatIndex);
	enqueueRhythmReservation(changedRhythm, applyAtBeatIndex);
}

void NetRecvSystem::RecvEmote(uint32 sessionId, const C2S_EmotePacket& pkt)
{
	Entity playerEntity = FindEntityBySession(sessionId);
	if (!playerEntity.IsValid())
		return;

	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(playerEntity);
	EmoteComponent* emoteComp = mWorld->GetComponent<EmoteComponent>(playerEntity);
	if (netComp == nullptr || playerComp == nullptr || emoteComp == nullptr ||
		netComp->mSessionId != sessionId)
		return;

	if (playerComp->IsDeathActive() || pkt.emoteId >= EMOTE_COUNT)
		return;

	constexpr float EmoteCooldownSeconds = 1.5f;
	const float now = GetServerTotalTimeSeconds();
	if (now - emoteComp->mLastRequestTime < EmoteCooldownSeconds)
	{
		return;
	}

	emoteComp->mLastRequestTime = now;

	if (auto eventManager = mWorld->GetEventManager())
	{
		eventManager->Enqueue(EvEmoteBroadcast{ playerEntity, pkt.emoteId });
	}
}

void NetRecvSystem::RecvChat(uint32 sessionId, const C2S_ChatPacket& pkt)
{
	Entity playerEntity = FindEntityBySession(sessionId);
	if (!playerEntity.IsValid())
		return;

	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (netComp == nullptr || netComp->mSessionId != sessionId)
		return;


	constexpr float ChatCooldownSeconds = 0.5f;		// 도배 방지
	const float now = GetServerTotalTimeSeconds();
	ChatComponent* chatComp = mWorld->GetComponent<ChatComponent>(playerEntity);
	if (chatComp)
	{
		if (now < chatComp->mNextChatTime)
			return;
		chatComp->mNextChatTime = now + ChatCooldownSeconds;
	}

	EvChatBroadcast ev{};
	ev.caster = playerEntity;
	std::memcpy(ev.text, pkt.text, sizeof(ev.text));
	ev.text[CHAT_TEXT_CAPACITY - 1] = L'\0';

	if (ev.text[0] == L'\0')
		return;

	if (auto eventManager = mWorld->GetEventManager())
	{
		eventManager->Enqueue(ev);
	}
}

// ─── 시간 동기 (공유 Song Clock) ─────────────────────────────
void NetRecvSystem::RecvSync(uint32 sessionId, const C2S_SyncPacket& pkt)
{
	// RTT 값 측정

	auto systemManager = mWorld->GetSystemManager();
	if (systemManager == nullptr)
		return;

	BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>();
	if (beatSystem == nullptr)
		return;

	S2C_SyncPacket resp{};
	resp.clientId = pkt.clientId;
	resp.serverSongPos = beatSystem->GetSongPosition();
	resp.clientEchoTime = pkt.SendTime;   // 클라 송신 시각 그대로 echo


	SendRequest req{ sessionId, PKT_Type::S2C_PKT_SYNC, sizeof(S2C_SyncPacket) };
	req.StoreAs<S2C_SyncPacket>(resp);
	gSendQueue.Push(req);
}

// ─── 게임 시작 처리 ──────────────────────────────────────────

void NetRecvSystem::HandleGameStart(InputCommand& inputCommand)
{
	Entity playerEntity = SpawnPlayer(inputCommand);
	if (!playerEntity.IsValid())
		return;

	EnsureBulletPool(inputCommand);

	// 최초 스폰 위치에 부활
	if (auto eventManager = mWorld->GetEventManager())
	{
		if (TransformComponent* spawnTransform = mWorld->GetComponent<TransformComponent>(playerEntity))
		{
			EvEffectSpawn fx{};
			fx.effectType = 0;
			fx.x = spawnTransform->mLocalPosition.x;
			fx.y = spawnTransform->mLocalPosition.y;
			fx.z = spawnTransform->mLocalPosition.z;
			fx.reason = EffectSpawnReason::Respawn;
			eventManager->Enqueue<EvEffectSpawn>(fx);
		}
	}

	auto spawnOneEnemy = [&](EnemyType enemyType, const Vec3& localOffset)
	{
		InputCommand spawnCmd{};
		spawnCmd.SessionId = 0;
		spawnCmd.Type = PKT_Type::S2C_PKT_SPAWN;
		spawnCmd.StoreAs(EnemySpawnContext{ static_cast<uint8>(enemyType) });

		Entity enemyEntity = PrefabFactory::Spawn(mWorld, PrefabType::ENEMY, spawnCmd);
		if (!enemyEntity.IsValid())
			return;

		if (TransformComponent* playerTransform = mWorld->GetComponent<TransformComponent>(playerEntity))
		{
			Vec3 spawnPos = playerTransform->mLocalPosition + localOffset;
			if (auto& physicsWorld = mWorld->GetPhysicsWorld())
			{
				float ground = 0.0f;
				if (physicsWorld->TryQueryTerrainHeightNear(spawnPos, spawnPos.y, 100.0f, 100.0f, ground))
					spawnPos.y = ground;
			}

			if (TransformComponent* enemyTransform = mWorld->GetComponent<TransformComponent>(enemyEntity))
			{
				enemyTransform->mLocalPosition = spawnPos;
				enemyTransform->mWorldMatrix = Matrix::CreateTranslation(spawnPos);
			}
			if (GravityComponent* enemyGravity = mWorld->GetComponent<GravityComponent>(enemyEntity))
			{
				enemyGravity->mGround = spawnPos.y;
				enemyGravity->mHight = spawnPos.y;
				enemyGravity->mGravity = 0.0f;
				enemyGravity->mFalling = false;
				enemyGravity->mDropping = false;
				enemyGravity->mGroundGraceLeft = 0.0f;
			}
			if (FlyComponent* flyComponent = mWorld->GetComponent<FlyComponent>(enemyEntity))
			{
				flyComponent->mGround = spawnPos.y;
			}
		}

		BroadcastEnemySpawn(mWorld, enemyEntity);
	};

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

	Entity playerEntity = PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, inputCommand);

	// 페이즈가 없는 씬(광장)에서도 스폰 지점에 배치되도록 스폰 직후 즉시 적용.
	ApplyPlayerSpawnPosition(mWorld, playerEntity);

	return playerEntity;
}

// ─── 불릿 풀 스폰 ────────────────────────────────────────────

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
