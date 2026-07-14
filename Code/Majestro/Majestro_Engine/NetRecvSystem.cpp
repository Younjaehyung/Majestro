#include "pch.h"
#include "NetRecvSystem.h"
#include "EnginePch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "World.h"
#include "Timer.h"
#include "InputManager.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "BulletComponent.h"
#include "NetIdMap.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "NetTransformComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "RhythmEmissiveComponent.h"
#include "TagComponent.h"
#include "BoxColliderComponent.h"
#include "DecalFactory.h"
#include "NetSendSystem.h"
#include "MovementSystem.h"
#include "VfxSystem.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "ComboComponent.h"
#include "VfxComponent.h"
#include "GameRuleComponent.h"
#include "PlayerStatusComponent.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "BeatSystem.h"

NetRecvSystem::NetRecvSystem(World* world,  shared_ptr<NetIdMap>& netIdMap)
	: System::System(world)
{
	mNetIdMap = netIdMap;
    mPhase = SysPhase::Pre;
}

NetRecvSystem::~NetRecvSystem()
{
}

void NetRecvSystem::Initialize()
{
    RegisterHandlers();

    if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
    if (entities.empty())return;
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
        mWorld->NetIdBinding(netComp->mNetEntityId, entity);
	}
}

void NetRecvSystem::RegisterHandlers()
{
    auto reg = [&](PKT_Type type, Handler h) {
        mHandlers[static_cast<size_t>(type)] = std::move(h);
    };

    reg(PKT_Type::S2C_PKT_SPAWN,             [this](auto& m){ HandleSpawn(m); });
    reg(PKT_Type::S2C_PKT_MOVE,              [this](auto& m){ HandleMove(m); });
    reg(PKT_Type::S2C_PKT_STATE,             [this](auto& m){ HandleState(m); });
    reg(PKT_Type::S2C_PKT_HEALTH,            [this](auto& m){ HandleHealth(m); });
    reg(PKT_Type::S2C_PKT_ARMOR,             [this](auto& m){ HandleArmor(m); });
    reg(PKT_Type::S2C_PKT_AMMO,              [this](auto& m){ HandleAmmo(m); });
    reg(PKT_Type::S2C_PKT_PLAYER_STATUS,     [this](auto& m){ HandlePlayerStatus(m); });
    reg(PKT_Type::S2C_PKT_COOLDOWN,          [this](auto& m){ HandleCooldown(m); });
    reg(PKT_Type::S2C_PKT_COLLISION,         [this](auto& m){ HandleCollision(m); });
    reg(PKT_Type::S2C_PKT_BULLET_ACTIVATE,   [this](auto& m){ HandleBulletActivate(m); });
    reg(PKT_Type::S2C_PKT_BULLET_DEACTIVATE, [this](auto& m){ HandleBulletDeactivate(m); });
    reg(PKT_Type::S2C_PKT_EFFECT_SPAWN,      [this](auto& m){ HandleEffectSpawn(m); });
    reg(PKT_Type::S2C_PKT_HIT_CONFIRM,       [this](auto& m){ HandleHitConfirm(m); });
    reg(PKT_Type::S2C_GAME_START,            [this](auto& m){ HandleGameStart(m); });
	reg(PKT_Type::S2C_SCENE_CHANGE_RESULT,   [this](auto& m){ HandleSceneChangeResult(m); });
	reg(PKT_Type::S2C_PKT_SCENE_STATE,       [this](auto& m) { HandleSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_PREPARE, [this](auto& m) { HandlePrepareSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_CONQUEST, [this](auto& m) { HandleConquestSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_ESCORT, [this](auto& m) { HandleEscortSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_CLEAR, [this](auto& m) { HandleClearSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCORE_BOARD, [this](auto& m) { HandleScoreBoard(m); });
	reg(PKT_Type::S2C_ROOM_STATE, [this](auto& m) { HandleRoomState(m); });
	reg(PKT_Type::S2C_ROOM_ERROR, [this](auto& m) { HandleRoomError(m); });
	reg(PKT_Type::S2C_ROOM_LIST, [this](auto& m) { HandleRoomList(m); });
	reg(PKT_Type::S2C_ROOM_JOIN_RESULT, [this](auto& m) { HandleRoomJoinResult(m); });
	reg(PKT_Type::S2C_PKT_DESPAWN, [this](auto& m) { HandleDespawn(m); });
	reg(PKT_Type::S2C_PKT_GIMMICK_STATE, [this](auto& m) { HandleGimmickState(m); });
	reg(PKT_Type::S2C_PKT_RHYTHM_CHANGED, [this](auto& m) { HandleRhythmChanged(m); });
	reg(PKT_Type::S2C_PKT_SYNC, [this](auto& m) { HandleSync(m); });
	reg(PKT_Type::S2C_PKT_BEAT_JUDGEMENT, [this](auto& m) { HandleBeatJudgement(m); });
	reg(PKT_Type::S2C_PKT_COMBO_CHANGED, [this](auto& m) { HandleComboChanged(m); });
	reg(PKT_Type::S2C_PKT_STICKER, [this](auto& m) { HandleSticker(m); });
}

void NetRecvSystem::Update(float deltaTime)
{

    constexpr int kMaxMsgsPerTick = 256; // 폭주 방지
    int processed = 0;
    mStopProcessing = false;
    
    while (processed < kMaxMsgsPerTick && gRecvBuffer.Pop(mInputCommand))
    {
        
        ProcessOne(mInputCommand);
        ++processed;
        if (mStopProcessing)
            break;
    }

   /* if (mWorld && mCmd)
        mCmd->Flush(*mWorld);*/
}

void NetRecvSystem::ProcessOne(const InputCommand& msg)
{
    const size_t idx = static_cast<size_t>(msg.Type);
    if (idx < mHandlers.size() && mHandlers[idx])
    {
        mHandlers[idx](msg);
        return;
    }
#ifdef _DEBUG
    std::cout << "[NetRecvSystem] 미등록 패킷 타입: " << static_cast<int>(msg.Type) << std::endl;
#endif
}

void NetRecvSystem::HandleMove(const InputCommand& msg)
{
    const S2C_MovePacket* pkt = msg.ViewAs<S2C_MovePacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    TransformComponent* transform = mWorld->GetComponent<TransformComponent>(e);
    NetTransformComponent* netTransform = mWorld->GetComponent<NetTransformComponent>(e);
    if (!transform || !netTransform) return;

    // 오래된 UDP 패킷 무시: out-of-order 도착한 패킷은 버림
    if (netTransform->mLastSequence != 0 &&
        !NetTransformComponent::IsNewer(pkt->Sequence, netTransform->mLastSequence))
        return;

    netTransform->mLastSequence   = pkt->Sequence;
    netTransform->mLastUpdateTime = TIMER.GetTotalTime();
    netTransform->mVelocity = Vec3(pkt->vx, pkt->vy, pkt->vz);

    if (mWorld->GetComponent<LocalPlayerComponent>(e))
    {
        // 로컬 플레이어: 현재 위치 → 서버 보정 위치로 Lerp
        // mStartPosition을 현재 위치로 갱신해야 올바른 구간에서 보간됨
        netTransform->mStartPosition   = transform->mLocalPosition;
        netTransform->mStartRotation   = transform->mLocalRotationE;
        netTransform->mTargetPosition  = { pkt->x, pkt->y, pkt->z };
        netTransform->mTargetRotation  = { pkt->pitch, pkt->yaw, 0.f };
        netTransform->mElapsed         = 0.0f;
        netTransform->mHasTarget       = true;
    }
    else
    {
        // 원격 엔티티: 스냅샷 버퍼에 삽입 → NetInterpolationSystem이 매 프레임 UpdateRender로 보간/외삽
        NetSnapshot snapshot;
        snapshot.serverTick = pkt->Sequence;
        snapshot.pos        = { pkt->x, pkt->y, pkt->z };
        snapshot.vel        = { pkt->vx, pkt->vy, pkt->vz };
        snapshot.rotQ       = { pkt->rx, pkt->ry, pkt->rz, pkt->rw };
        netTransform->OnSnapshot(snapshot, static_cast<double>(TIMER.GetTotalTime()));
#ifdef _DEBUG
        static double sLastRecvTime = 0.0;
        const double now = static_cast<double>(TIMER.GetTotalTime());
       /* std::cout << "[MOVE] interval=" << (now - sLastRecvTime) * 1000.0 << "ms"
                  << " bufSize=" << netTransform->mBuffer.size()
                  << " interpDelayTicks=" << netTransform->mInterpDelayTicksF << std::endl;*/
        sLastRecvTime = now;
#endif
    }
}

void NetRecvSystem::HandleState(const InputCommand& msg)
{
    const S2C_StatePacket* pkt = msg.ViewAs<S2C_StatePacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
    EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(e);
    NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
    NetTransformComponent* netTransform = mWorld->GetComponent<NetTransformComponent>(e);
    if (!netComp) return;

    if (playerComp)
    {
        playerComp->mPrevStatePacket = playerComp->mUpperState;
        playerComp->mPrevLowerStatePacket = playerComp->mLowerState;
        playerComp->mPrevStateSequence = playerComp->mStateSequence;

        playerComp->mUpperState = pkt->stateId;
        playerComp->mLowerState = pkt->lowerStateId;
        playerComp->mControlFlags = pkt->controlFlags;
        playerComp->mExternalMoveMode = pkt->externalMoveMode;
        playerComp->mStateSequence = pkt->stateSequence;

        // 리듬 변경 이미시브
        const int rhythmChange = static_cast<int>(ReplicatedActionState::RhythmChange);
        if (playerComp->mUpperState == rhythmChange && playerComp->mPrevStatePacket != rhythmChange)
        {
            auto* rhythmEmissive = mWorld->GetComponent<RhythmEmissiveComponent>(e);
            rhythmEmissive->mTimer = rhythmEmissive->mDuration;
        }
    }
    else if (enemyComp)
    {
        enemyComp->mAnimState = pkt->stateId;
    }

    if (netTransform)
        netTransform->mElapsed = 0.0f;


}

void NetRecvSystem::HandleRhythmChanged(const InputCommand& msg)
{
    const S2C_RhythmChangedPacket* pkt = msg.ViewAs<S2C_RhythmChangedPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
    if (!playerComp) return;

    playerComp->mNextRhythm = NormalizeRhythm(pkt->changedRhythm);
    playerComp->mRhythm = playerComp->mNextRhythm;
    playerComp->mRhythmApplyBeat = pkt->applyAtBeatIndex;
    playerComp->mHasQueuedRhythmChange = true;
    playerComp->mDesiredRhythm = playerComp->mNextRhythm;
    playerComp->mRhythmSettleTimer = 0.f;
    playerComp->mRhythmChangeInFlight = false;
}

// 공유 Song Clock: 서버와 클라 간 시간 동기화. 서버가 보낸 곡 위치 + 편도 지연으로 클라 곡 위치 보정
void NetRecvSystem::HandleSync(const InputCommand& msg)
{
    const S2C_SyncPacket* pkt = msg.ViewAs<S2C_SyncPacket>();
    if (!pkt) return;

    auto systemManager = mWorld->GetSystemManager();
    if (!systemManager) return;

    BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>();
    if (!beatSystem) return;

    const double now = static_cast<double>(TIMER.GetTotalTime());
    const double rtt = now - pkt->clientEchoTime;
    // 비정상 RTT 방어 후 편도 지연 값 추정  
    const float halfRtt = static_cast<float>((std::max)(0.0, (std::min)(rtt, 1.0)) * 0.5);

    // 응답 도착 시점의 서버 곡 위치
    const float estimatedServerSongPos = pkt->serverSongPos + halfRtt;  // 서버가 보낸 곡 위치 + 편도 지연
    beatSystem->SyncSongPosition(estimatedServerSongPos);
}

// 서버 박자 판정 수신
void NetRecvSystem::HandleBeatJudgement(const InputCommand& msg)
{
    const S2C_BeatJudgementPacket* pkt = msg.ViewAs<S2C_BeatJudgementPacket>();
    if (!pkt) return;

    // 클라 즉시 예측(predicted=true)을 서버 결과(predicted=false)로 보정
    mWorld->GetEventManager()->Enqueue(EvBeatJudgement{ pkt->judgement, pkt->actionButton, false });

    std::cout << "[BeatJudge/server] btn " << static_cast<int>(pkt->actionButton)
              << " => " << static_cast<int>(pkt->judgement) << std::endl;
}


void NetRecvSystem::HandleComboChanged(const InputCommand& msg)
{
    const S2C_ComboChangedPacket* pkt = msg.ViewAs<S2C_ComboChangedPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    if (e == NULL_ENTITY) return;


    ComboComponent* combo = mWorld->GetComponent<ComboComponent>(e);
    if (combo == nullptr)
        combo = &mWorld->AddComponent<ComboComponent>(e);

    combo->mCount = pkt->comboCount;
}

void NetRecvSystem::HandleHealth(const InputCommand& msg)
{
    const S2C_HealthPacket* pkt = msg.ViewAs<S2C_HealthPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    HealthComponent* healthComp = mWorld->GetComponent<HealthComponent>(e);
    if (!healthComp) return;

    //std::cout << "[Client][S2C_PKT_HEALTH] netEntityId=" << pkt->netEntityId
    //    << " hp=" << healthComp->mCurrentHp << "/" << healthComp->mMaxHp
    //    << " -> " << pkt->currentHp << "/" << pkt->maxHp << std::endl;


    const int32 previousHp = healthComp->mCurrentHp;
    healthComp->mCurrentHp = pkt->currentHp;
    healthComp->mMaxHp     = pkt->maxHp;

    // hit direction 계산.

    Vec3 hitDir = Vec3::Zero;
    if (pkt->attackerNetId != 0)
    {
        Entity attacker = mWorld->GetEntityByNetId(pkt->attackerNetId);
        TransformComponent* atkTr = mWorld->GetComponent<TransformComponent>(attacker);
        TransformComponent* tgtTr = mWorld->GetComponent<TransformComponent>(e);
        if (atkTr && tgtTr)
        {
            hitDir = tgtTr->mLocalPosition - atkTr->mLocalPosition;
            if (hitDir.LengthSquared() > 1e-6f)
                hitDir.Normalize();
            else
                hitDir = Vec3::Zero;
        }
    }

    mWorld->GetEventManager()->Enqueue(EvHealthChanged{
       e, pkt->currentHp, pkt->maxHp, previousHp, pkt->isCritical != 0, hitDir
		});
}

void NetRecvSystem::HandleArmor(const InputCommand& msg)
{
    const S2C_ArmorPacket* pkt = msg.ViewAs<S2C_ArmorPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    ArmorComponent* armorComp = mWorld->GetComponent<ArmorComponent>(e);
    if (!armorComp) return;

    

    armorComp->mCurrentArmor = pkt->currentArmor;
    armorComp->mMaxArmor     = pkt->maxArmor;

    mWorld->GetEventManager()->Enqueue(EvHpArmorChanged{
        e, pkt->currentArmor, pkt->maxArmor
        });
}

void NetRecvSystem::HandleAmmo(const InputCommand& msg)
{
    const S2C_AmmoPacket* pkt = msg.ViewAs<S2C_AmmoPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
    if (!playerComp) return;

    playerComp->mNowBullet = pkt->currentAmmo;
    playerComp->mMaxBullet = pkt->maxAmmo;


    mWorld->GetEventManager()->Enqueue(EvBulletCountChanged{
    e, pkt->currentAmmo, pkt->maxAmmo
        });
}

void NetRecvSystem::HandlePlayerStatus(const InputCommand& msg)
{
	const S2C_PlayerStatusPacket* pkt = msg.ViewAs<S2C_PlayerStatusPacket>();
	if (!pkt) return;

	Entity entity = mWorld->GetEntityByNetId(pkt->netEntityId);
	if (entity == NULL_ENTITY || !mWorld->HasComponent<LocalPlayerComponent>(entity))
		return;

	PlayerStatusComponent* status = mWorld->GetComponent<PlayerStatusComponent>(entity);
	if (!status)
		status = &mWorld->AddComponent<PlayerStatusComponent>(entity);

	status->mStatusFlags = pkt->statusFlags;
	status->mStunRemaining = pkt->stunRemaining;
	status->mRespawnRemaining = pkt->respawnRemaining;
	status->mBuffCount = std::min<uint8>(pkt->buffCount, MAX_REPLICATED_BUFFS);

	for (uint8 index = 0; index < MAX_REPLICATED_BUFFS; ++index)
	{
		status->mBuffs[index] =
			index < status->mBuffCount
			? pkt->buffs[index]
			: ReplicatedBuffState{};
	}
}

void NetRecvSystem::HandleCooldown(const InputCommand& msg)
{
    const S2C_CooldownPacket* pkt = msg.ViewAs<S2C_CooldownPacket>();
    if (!pkt) return;
    if (pkt->skillSlot >= MainPlayerComponent::kCooldownSlotCount) return;

    for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        if (!mWorld->HasComponent<LocalPlayerComponent>(e))
            continue;

        MainPlayerComponent* mp = mWorld->GetComponent<MainPlayerComponent>(e);

        const float nowLocal = TIMER.GetTotalTime();
        mp->mCooldownDuration[pkt->skillSlot] = pkt->durationSeconds;
        mp->mCooldownEndLocal[pkt->skillSlot] = nowLocal + pkt->remainingSeconds;
        break;
    }
}


void NetRecvSystem::HandleCollision(const InputCommand& msg)
{
    const S2C_CollisionPacket* pkt = msg.ViewAs<S2C_CollisionPacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    BoxColliderComponent* boxComp = mWorld->GetComponent<BoxColliderComponent>(e);
    if (!boxComp) return;

    boxComp->bIsColliding = pkt->bIsColliding;
}

void NetRecvSystem::HandleBulletActivate(const InputCommand& msg)
{
    const S2C_BulletActivatePacket* pkt = msg.ViewAs<S2C_BulletActivatePacket>();
    if (!pkt) return;

    Entity bulletEntity = mWorld->GetEntityByNetId(pkt->bulletNetEntityId);
    if (bulletEntity == NULL_ENTITY)
    {
        std::cout << "Bullet Activate: 엔티티 없음 - bullet: " << pkt->bulletNetEntityId << std::endl;
        return;
    }

    BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
    TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
    if (!bulletComp || !bulletTransform) return;

    Vec3 spawnPosition{ pkt->x, pkt->y, pkt->z };
    Vec3 direction{ pkt->dirX, pkt->dirY, pkt->dirZ };
    if (direction.LengthSquared() <= 0.0001f)
        direction = Vec3::Forward;
    direction.Normalize();

    // 네트워크 지연만큼 탄환 위치를 앞당겨 보정 (최대 0.25초 상한)
    const double nowSec = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const float compensationSec = static_cast<float>(
        (std::min)((std::max)(0.0, nowSec - pkt->SendTime), 0.25));
    spawnPosition += direction * pkt->speed * compensationSec;

    bulletTransform->mLocalPosition = spawnPosition;
    bulletTransform->mWorldPosition = spawnPosition;
    // Fix: use server bullet size for client visual state instead of prefab defaults.
    bulletTransform->mLocalScale = Vec3(pkt->size, pkt->size, pkt->size);
    // Fix: use the server-computed bullet rotation so impact VFX receives a direction-correct rot value.
    bulletTransform->mLocalRotationE = Vec3(pkt->rotX, pkt->rotY, pkt->rotZ);

    SkillType bulletType = static_cast<SkillType>(pkt->bulletType);
    if (bulletType >= SkillType::Max)
        bulletType = SkillType::Default;

    bulletComp->Activate(
        bulletType,
        pkt->ownerNetEntityId,
        static_cast<uint32>(pkt->bulletNetEntityId),
        pkt->bulletGeneration,
        spawnPosition,
        direction,
        pkt->speed,
        pkt->lifeTime,
        bulletComp->mDamage);

    bulletComp->mElapsedTime = (std::min)(compensationSec, bulletComp->mLifeTime);

    mWorld->GetEventManager()->Enqueue(EvAttachBulletVfx{ bulletEntity, bulletType, pkt->bulletGeneration });

    if (auto movementSystem = mWorld->GetSystemManager()->GetSystem<MovementSystem>())
        movementSystem->RegisterActiveBullet(bulletEntity);
}

void NetRecvSystem::HandleBulletDeactivate(const InputCommand& msg)
{
    const S2C_BulletDeactivatePacket* pkt = msg.ViewAs<S2C_BulletDeactivatePacket>();
    if (!pkt) return;

    Entity bulletEntity = mWorld->GetEntityByNetId(pkt->bulletNetEntityId);
    if (bulletEntity == NULL_ENTITY) return;

    BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
    TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
    VfxComponent* bulletVfx = mWorld->GetComponent<VfxComponent>(bulletEntity);
    if (!bulletComp || !bulletTransform) return;

    if (pkt->bulletGeneration != bulletComp->mGeneration)
        return;

    bulletComp->Deactivate();
    if (bulletVfx)
    {
      
        bulletVfx->mShouldPlay = false;
        bulletVfx->mIsPaused = false;
        bulletVfx->mScale = Vec3::Zero;
        bulletVfx->mIsPlaying = false;
        bulletVfx->mTotalTime = 0.f;
    }
    bulletTransform->mMovingVector = Vec3::Zero;

    if (auto movementSystem = mWorld->GetSystemManager()->GetSystem<MovementSystem>())
        movementSystem->UnregisterActiveBullet(bulletEntity);
}

void NetRecvSystem::HandleSticker(const InputCommand& msg)
{
    const S2C_StickerPacket* pkt = msg.ViewAs<S2C_StickerPacket>();
    if (!pkt) return;

    const Vec3 camPos(pkt->camX, pkt->camY, pkt->camZ);
    const Vec3 forward(pkt->dirX, pkt->dirY, pkt->dirZ);

    // textureId  : 셀 인덱스
    const int emoteIndex = static_cast<int>(pkt->textureId % EmoteSticker::kAtlasCount);

    const std::wstring texName = L"DecalSticker";
    DecalFactory::StampSurfaceSticker(mWorld, camPos, forward, texName, pkt->size, -1.0f,
                                      EmoteSticker::kAtlasGrid, emoteIndex);
}

void NetRecvSystem::HandleEffectSpawn(const InputCommand& msg)
{
    const S2C_EffectSpawnPacket* pkt = msg.ViewAs<S2C_EffectSpawnPacket>();
    if (!pkt) return;

    SkillType skillType = static_cast<SkillType>(pkt->effectType);
    if (skillType >= SkillType::Max)
        skillType = SkillType::Default;

    mWorld->GetEventManager()->Enqueue(EvVfxSpawnRequest{
        skillType,
        pkt->reason,
        Vec3(pkt->x, pkt->y, pkt->z),
        Vec3(pkt->rotX, pkt->rotY, pkt->rotZ),
        pkt->casterNetId });

    const bool pianoAttackDebug = skillType == SkillType::PianoAttack &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::LifetimeExpired);

    const bool slimeAttackDebug = skillType == SkillType::SlimeAttack &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::Fire);

    const bool flyAttackDebug = skillType == SkillType::FlyAttack &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::Fire);

    const bool bongoAttackDebug = skillType == SkillType::BongoAttack &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::Fire);

    const bool dragonAttackDebug =
        (skillType == SkillType::DragonSkill1 ||
         skillType == SkillType::DragonSkill3 ||
         skillType == SkillType::DragonSkill4) &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::Fire);

	    const bool playerMeleeDebug =
	        (skillType == SkillType::DrumAttack ||
	         skillType == SkillType::DrumAttack3 ||
	         skillType == SkillType::DrumSkill1 ||
	         skillType == SkillType::GuitarAttack ||
	         skillType == SkillType::GuitarSkill1) &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::Fire);

    const bool guitarAttack2ExplosionDebug =
        skillType == SkillType::GuitarAttack_2 &&
        pkt->reason == static_cast<uint8>(EffectSpawnReason::CollisionEntity);

    if (pianoAttackDebug || slimeAttackDebug || flyAttackDebug || bongoAttackDebug || dragonAttackDebug || playerMeleeDebug || guitarAttack2ExplosionDebug)
    {
        mWorld->GetEventManager()->Enqueue(EvEnemyAttackDebug{
            skillType,
            pkt->reason,
            Vec3(pkt->x, pkt->y, pkt->z),
            Vec3(pkt->rotX, pkt->rotY, pkt->rotZ),
            !pianoAttackDebug });
    }
}

void NetRecvSystem::HandleHitConfirm(const InputCommand& msg)
{
    const S2C_HitConfirmPacket* pkt = msg.ViewAs<S2C_HitConfirmPacket>();
    if (!pkt) return;

    EvHitMarker ev{};
    ev.damage = pkt->damage;
    ev.isKill = (pkt->isKill != 0);
    mWorld->GetEventManager()->Enqueue(ev);
}

void NetRecvSystem::HandleGameStart(const InputCommand& msg)
{
    cout << "GameStart" << endl;
    mStopProcessing = true;
	gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::Plaza, L"광장 로딩 중...");
}

void NetRecvSystem::HandleSceneChangeResult(const InputCommand& msg)
{
    const S2C_SceneChangeResultPacket* resultPacket = msg.ViewAs<S2C_SceneChangeResultPacket>();
    if (resultPacket == nullptr)
        return;

    if (!resultPacket->approved)
        return;

    if (mCurrentScene == resultPacket->currentScene)
        return;

    mCurrentScene = resultPacket->currentScene;

    mStopProcessing = true;
    switch (mCurrentScene)
    {
    case SceneId::Lobby:
		// 로비는 즉시 로드(RequestScene)로 전환
		gEngine->GetSceneManager().RequestScene(SceneId::Lobby);
        break;
    case SceneId::Plaza: // 광장
        if (auto sendSystem = mWorld->GetSystemManager()->GetSystem<NetSendSystem>())
            sendSystem->RequestPendingGameStart();
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::Plaza, L"광장 로딩 중...");
        break;
    case SceneId::FirstGame:
        if (auto sendSystem = mWorld->GetSystemManager()->GetSystem<NetSendSystem>())
            sendSystem->RequestPendingGameStart();
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::FirstGame, L"게임 씬 로딩 중...");
        break;
    case SceneId::SecondGame: // SecondScene 으로 교체
        if (auto sendSystem = mWorld->GetSystemManager()->GetSystem<NetSendSystem>())
            sendSystem->RequestPendingGameStart();
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::SecondGame, L"다음 스테이지 로딩 중...");
        break;
    case SceneId::ThirdGame: // ThirdScene 으로 교체
        if (auto sendSystem = mWorld->GetSystemManager()->GetSystem<NetSendSystem>())
            sendSystem->RequestPendingGameStart();
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::ThirdGame, L"보스전 로딩 중...");
        break;
    case SceneId::VGame: // 승리 화면
        Network::GetInstance().Shutdown();
        gEngine->GetSceneManager().RequestScene(SceneId::VGame);
        break;
    case SceneId::MainMenu: // 게임 종료 신호
        // 게임 중에는 마우스 룩으로 커서를 숨겨두므로, 메인 메뉴에서 다시 커서가 보이도록 복원한다.
        INPUT.SetForceMouseLook(false);
        // 스스로 접속을 끊고 메인 메뉴로 복귀
        Network::GetInstance().Shutdown();
        gEngine->GetSceneManager().RequestScene(SceneId::MainMenu);
        break;
    default:
        break;
    }
}

void NetRecvSystem::HandleSceneState(const InputCommand& msg)
{
    const S2C_SceneStatePacket* pkt = msg.ViewAs<S2C_SceneStatePacket>();
     if (!pkt) return;
	 // 씬 상태 정보 처리 (예: 라운드 시작, 종료, 점수 업데이트 등)

	 GameRuleComponent* gameRuleComp = mWorld->GetSingleton<GameRuleComponent>();
	 if (!gameRuleComp) return;

	 gameRuleComp->mGameTime = pkt->GameTime;
	 gameRuleComp->mPlayerScore = pkt->PlayerScore;

	 if (gameRuleComp->mGamePhase != pkt->GamePhase)
		 mWorld->GetEventManager()->Enqueue(EvGamePhaseChanged{ gameRuleComp->mGamePhase, pkt->GamePhase });

	 gameRuleComp->mGamePhase = pkt->GamePhase;
}

void NetRecvSystem::HandlePrepareSceneState(const InputCommand& msg)
{
	const S2C_PreparePacket* pkt = msg.ViewAs<S2C_PreparePacket>();
	if (!pkt) return;

	GamePrepareComponent* prepareComp = mWorld->GetSingleton<GamePrepareComponent>();
	if (!prepareComp)
		prepareComp = &mWorld->AddSingleton<GamePrepareComponent>();

	prepareComp->mReadyPlayers = pkt->ReadyPlayers;
	prepareComp->mTotalPlayers = pkt->TotalPlayers;
	prepareComp->mReadyCheckRemaining = pkt->ReadyCheckRemaining;
	prepareComp->mForcedStartRemaining = pkt->ForcedStartRemaining;
	prepareComp->mCountdownRemaining = pkt->CountdownRemaining;
	prepareComp->mCountdownStarted = pkt->CountdownStarted != 0;
	prepareComp->mAllPlayersReady = pkt->AllPlayersReady != 0;
}

void NetRecvSystem::HandleConquestSceneState(const InputCommand& msg)
{
    const S2C_ConquestPacket* pkt = msg.ViewAs<S2C_ConquestPacket>();
	if (!pkt) return;
	// Conquest 씬 상태 정보 처리 (예: 점령 상태, 팀 점수 등)
    
	GameConquestComponent* conquestComp = mWorld->GetSingleton<GameConquestComponent>();
	if (!conquestComp) return;

	conquestComp->mActiveZoneId = pkt->ActiveZoneId;
	conquestComp->mActiveZonePos = Vec3(pkt->ZoneX, pkt->ZoneY, pkt->ZoneZ);
	conquestComp->mWaveCheckPoint = pkt->WaveCheckPoint;
	conquestComp->mWave = pkt->Wave;
	conquestComp->mWaveInterval = pkt->WaveInterval;
	conquestComp->mWaveTime = pkt->WaveTime;
	conquestComp->mRequiredConquestTime = pkt->RequiredConquestTime;
	conquestComp->mPlayerNum = pkt->PlayerNum;
	conquestComp->mEnemyNum = pkt->EnemyNum;

}

void NetRecvSystem::HandleEscortSceneState(const InputCommand& msg)
{
    const S2C_EscortPacket* pkt = msg.ViewAs<S2C_EscortPacket>();
    if (!pkt) return;
    // Escort 씬 상태 정보 처리 (예: 호위 대상 HP, 남은 시간 등)
    GameEscortComponent* escortComp = mWorld->GetSingleton<GameEscortComponent>();
    if (!escortComp) return;
    //escortComp->mRouteId = pkt->RouteId;
    escortComp->mEscortProgress = pkt->EscortProgress;
	escortComp->mEscortStage = pkt->EscortStage;
	escortComp->mStageCount = pkt->StageCount;
	escortComp->mStageProgress = pkt->StageProgress;
	escortComp->mEscortTime = pkt->EscortTime;
	escortComp->mMoveState = pkt->MoveState;
	
	if (pkt->TruckNetId != 0)
	{
		Entity truck = mWorld->GetEntityByNetId(pkt->TruckNetId);
		if (truck != NULL_ENTITY)
			escortComp->mEscortTarget = truck;
	}
}

void NetRecvSystem::HandleClearSceneState(const InputCommand& msg)
{
	const S2C_ClearPacket* pkt = msg.ViewAs<S2C_ClearPacket>();
	if (!pkt) return;

	GameClearComponent* clearComp = mWorld->GetSingleton<GameClearComponent>();
	if (!clearComp)
		clearComp = &mWorld->AddSingleton<GameClearComponent>();

	clearComp->mPlayerCount = std::min<uint8>(pkt->PlayerCount, ROOM_MAX_PLAYERS);
	clearComp->mTeamScore = pkt->TeamScore;
	clearComp->mGameTime = pkt->GameTime;

	for (uint8 index = 0; index < ROOM_MAX_PLAYERS; ++index)
	{
		clearComp->mSessionIds[index] = pkt->Players[index].SessionId;
		clearComp->mPlayerTypes[index] = pkt->Players[index].PlayerType;
	}
}

void NetRecvSystem::HandleScoreBoard(const InputCommand& msg)
{
	const S2C_ScoreBoardPacket* pkt = msg.ViewAs<S2C_ScoreBoardPacket>();
	if (!pkt)
		return;

	ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
	if (!scoreBoard)
		scoreBoard = &mWorld->AddSingleton<ScoreBoardComponent>();

	// Replace the local snapshot so disconnected players do not remain on the board.
	*scoreBoard = ScoreBoardComponent{};
	scoreBoard->mPlayerCount = (std::min)(pkt->PlayerCount, static_cast<uint8>(ROOM_MAX_PLAYERS));
	scoreBoard->mGameTime = pkt->GameTime;

	for (uint8 index = 0; index < scoreBoard->mPlayerCount; ++index)
	{
		const ScoreBoardPlayerInfo& source = pkt->Players[index];
		ClientPlayerScore& target = scoreBoard->mPlayers[index];
		target.mSessionId = source.SessionId;
		target.mPlayerType = source.PlayerType;
		target.mScore = source.Score;
		target.mTotalKills = source.TotalKills;
		target.mAssists = source.Assists;
		target.mMaxCombo = source.MaxCombo;
	}
}

// 서버 RoomState 스냅샷을 LobbyRoomStateComponent 에 복사.
void NetRecvSystem::HandleRoomState(const InputCommand& msg)
{
    const S2C_RoomStatePacket* pkt = msg.ViewAs<S2C_RoomStatePacket>();
    if (!pkt) return;

    // 로비 씬에서만
    if (!mWorld->HasComponentPool<LobbyRoomStateComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
    if (entities.empty()) return;

    LobbyRoomStateComponent* state = mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]);
    if (!state) return;

    // 내가 속한 방의 상태만 반영.
    if (auto* listComp = mWorld->GetComponent<LobbyRoomListComponent>(entities[0]))
    {
        if (listComp->mCurrentRoomId != 0 && pkt->roomId != listComp->mCurrentRoomId)
            return;

        const uint32 me = Network::GetInstance().mClientId;
        if (listComp->mCurrentRoomId == 0 && me != 0)
        {
            for (uint8 i = 0; i < pkt->maxPlayers && i < ROOM_MAX_PLAYERS; ++i)
            {
                if (pkt->slots[i].sessionId != 0 && pkt->slots[i].sessionId == me)
                {
                    listComp->mCurrentRoomId = pkt->roomId;

                    // 미러링
                    if (mWorld->HasComponentPool<ChoicePlayerComponent>())
                    {
                        auto choiceEnts = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
                        if (!choiceEnts.empty())
                            if (auto* choice = mWorld->GetComponent<ChoicePlayerComponent>(choiceEnts[0]))
                                choice->mPlayerType = pkt->slots[i].playerType;
                    }
                    break;
                }
            }
        }
    }

    state->mRoomId = pkt->roomId;
    state->mPlayerCount = pkt->playerCount;
    state->mMaxPlayers = pkt->maxPlayers;
    state->mHostSlotIndex = pkt->hostSlotIndex;

    const uint8 copyCount = (pkt->maxPlayers < ROOM_MAX_PLAYERS) ? pkt->maxPlayers : ROOM_MAX_PLAYERS;
    for (uint8 i = 0; i < copyCount; ++i)
    {
        state->mSlots[i].sessionId = pkt->slots[i].sessionId;
        state->mSlots[i].playerType = pkt->slots[i].playerType;
        state->mSlots[i].ready = (pkt->slots[i].ready != 0);
        state->mSlots[i].isHost = (pkt->slots[i].isHost != 0);
    }
    state->mHasSnapshot = true;

    // 미러링 2
    if (mWorld->HasComponentPool<ChoicePlayerComponent>())
    {
        auto choiceEnts = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
        if (!choiceEnts.empty())
        {
            if (auto* choice = mWorld->GetComponent<ChoicePlayerComponent>(choiceEnts[0]))
            {
                const uint32 me = Network::GetInstance().mClientId;
                uint8 mySlotType   = choice->mPlayerType;
                bool  foundMySlot  = false;
                bool  lockedByOther = false;
                for (uint8 i = 0; i < copyCount; ++i)
                {
                    const auto& s = state->mSlots[i];
                    if (s.sessionId == 0) continue;
                    if (s.sessionId == me)            { mySlotType = s.playerType; foundMySlot = true; }
                    else if (s.ready && s.playerType == choice->mPlayerType) lockedByOther = true;
                }
                if (foundMySlot && lockedByOther)
                    choice->mPlayerType = mySlotType;
            }
        }
    }
}

// 서버 거부 사유 수신
void NetRecvSystem::HandleRoomError(const InputCommand& msg)
{
    const S2C_RoomErrorPacket* pkt = msg.ViewAs<S2C_RoomErrorPacket>();
    if (!pkt) return;

    std::cout << "[RoomError] roomId=" << pkt->roomId
              << " code=" << static_cast<int>(pkt->errorCode) << std::endl;

    if (auto eventMgr = mWorld->GetEventManager())
        eventMgr->Enqueue(EvRoomError{ pkt->errorCode });
}

//  방 목록 수신
void NetRecvSystem::HandleRoomList(const InputCommand& msg)
{
    const S2C_RoomListPacket* pkt = msg.ViewAs<S2C_RoomListPacket>();
    if (!pkt) return;

    if (!mWorld->HasComponentPool<LobbyRoomListComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty()) return;

    LobbyRoomListComponent* listComp = mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
    if (!listComp) return;

    const uint8 count = (pkt->count < ROOM_LIST_MAX_ENTRIES) ? pkt->count : ROOM_LIST_MAX_ENTRIES;
    for (uint8 i = 0; i < count; ++i)
    {
        listComp->mEntries[i].roomId      = pkt->entries[i].roomId;
        listComp->mEntries[i].playerCount = pkt->entries[i].playerCount;
        listComp->mEntries[i].maxPlayers  = pkt->entries[i].maxPlayers;
        listComp->mEntries[i].phase       = pkt->entries[i].phase;
    }
    listComp->mCount = count;
    listComp->mHasList = true;
}

// 방 생성/입장 결과 수신
void NetRecvSystem::HandleRoomJoinResult(const InputCommand& msg)
{
    const S2C_RoomJoinResultPacket* pkt = msg.ViewAs<S2C_RoomJoinResultPacket>();
    if (!pkt) return;

    if (!mWorld->HasComponentPool<LobbyRoomListComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty()) return;

    LobbyRoomListComponent* listComp = mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
    if (!listComp) return;

    if (pkt->success != 0)
    {
        listComp->mCurrentRoomId = pkt->roomId;   // 대기실 진입

        // 새 방의 상태 스냅샷이 곧 도착하므로, 이전 방 스냅샷을 무효화
        if (auto* state = mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]))
            state->mHasSnapshot = false;
    }
    else
    {
        // 실패: 오류값 반환
        if (auto eventMgr = mWorld->GetEventManager())
            eventMgr->Enqueue(EvRoomError{ pkt->errorCode });
    }
}


void NetRecvSystem::HandleSpawn(const InputCommand& msg)
{

	uint32 netId = 0;
	uint32 archetypeId = 0;
	const S2C_SpawnPacekt* spawnPacket = msg.ViewAs<S2C_SpawnPacekt>();
    if (spawnPacket == nullptr) return;
	archetypeId = static_cast<uint32_t>(spawnPacket->prefabType);
	netId = static_cast<uint32_t>(spawnPacket->netEntityId);

	auto shouldRecreateExisting = [&](Entity existing) -> bool
	{
		if (existing == NULL_ENTITY)
			return false;

		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(existing);
		NetTransformComponent* netTransform = mWorld->GetComponent<NetTransformComponent>(existing);
		NetEntityComponent* netEntity = mWorld->GetComponent<NetEntityComponent>(existing);
		if (!transform || !netTransform || !netEntity)
			return true;

		switch (spawnPacket->prefabType)
		{
		case PrefabType::ENEMY:
		{
			EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(existing);
			RenderComponent* render = mWorld->GetComponent<RenderComponent>(existing);
			HealthComponent* health = mWorld->GetComponent<HealthComponent>(existing);
			if (!enemy || !render || !health)
				return true;
			if (enemy->mEnemyType != spawnPacket->Type)
				return true;
			break;
		}
		case PrefabType::PLAYER:
			if (!mWorld->GetComponent<MainPlayerComponent>(existing))
				return true;
			break;
		case PrefabType::BULLET:
			if (!mWorld->GetComponent<BulletComponent>(existing))
				return true;
			break;
		default:
			break;
		}

		return false;
	};

	Entity existing = mWorld->GetEntityByNetId(netId);
	const bool forceRecreateEnemy = (spawnPacket->prefabType == PrefabType::ENEMY && existing != NULL_ENTITY);
	if (forceRecreateEnemy || shouldRecreateExisting(existing))
	{
		mWorld->DestroyEntity(existing);
		mWorld->NetIdUnbinding(netId);
		existing = NULL_ENTITY;
	}

    if (existing == NULL_ENTITY) {
        Entity e = CreateEntityFromArchetype(archetypeId, msg);
        mWorld->NetIdBinding(netId, e);

        // 몬스터 스폰 VFX
        if (spawnPacket->prefabType == PrefabType::ENEMY && spawnPacket->hasInitialTransform != 0)
        {
            if (auto vfxSystem = mWorld->GetSystemManager()->GetSystem<VfxSystem>())
            {
                const Vec3 spawnPos(spawnPacket->x, spawnPacket->y + 100.f, spawnPacket->z);
                vfxSystem->PlayOneShot(L"VFX_Monster_Spawn", spawnPos, Vec3::Zero, Vec3(5.0f));
            }
        }
    }
    else {
        if (TransformComponent* transform = mWorld->GetComponent<TransformComponent>(existing))
        {
            std::cout << "[SpawnSkipPos] netId=" << netId
                << " entityId=" << existing.GetID()
                << " pos=(" << transform->mLocalPosition.x
                << ", " << transform->mLocalPosition.y
                << ", " << transform->mLocalPosition.z << ")"
                << std::endl;
        }
    }
       


    // 초기 상태도 같이 온다면 반영
    //NetTransformState nts{};
    //if (r.Read(nts)) {
    //    // 
    //    mCmd->SetNetTransform(e, nts);
    //}
}

void NetRecvSystem::HandleDespawn(const InputCommand& msg)
{
    const S2C_DespawnPacket* pkt = msg.ViewAs<S2C_DespawnPacket>();
    if (!pkt) return;

    const uint64 netId = pkt->netEntityId;
    Entity e = mWorld->GetEntityByNetId(netId);
    if (e == NULL_ENTITY) return;

    // 이탈/접속 종료한 캐릭터 제거
    mWorld->DestroyEntity(e);
    mWorld->NetIdUnbinding(netId);
}

void NetRecvSystem::HandleGimmickState(const InputCommand& msg)
{
    const S2C_GimmickStatePacket* pkt = msg.ViewAs<S2C_GimmickStatePacket>();
    if (!pkt) return;

    Entity e = mWorld->GetEntityByNetId(pkt->netEntityId);
    std::cout << "[Gimmick] state recv netId=" << pkt->netEntityId
        << " active=" << static_cast<int>(pkt->active)
        << (e == NULL_ENTITY ? " (엔티티 없음)" : "") << std::endl;

    if (e == NULL_ENTITY) return;

    // 기믹 엔티티는 파괴하지 않고 VFX 표시만 토글
    if (VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(e))
        vfx->mShouldPlay = (pkt->active != 0);
}

void NetRecvSystem::HandleReplicationDelta(const InputCommand& msg)
{

    uint32_t netId = 0;
    RepCompKind compKind{};
    uint32_t fieldMask = 0;

   /* if (!r.Read(netId)) return;
    if (!r.Read(compKind)) return;
    if (!r.Read(fieldMask)) return;*/

    Entity e = mWorld->GetEntityByNetId(netId);
    if (e == 0) {
        // 아직 Spawn이 안 왔거나, 관심영역 늦게 들어온 케이스
        // 실전에서는 여기서 "Spawn 요청" 또는 "임시 보류" 전략을 둠
        return;
    }

	
    //mEventManager->PushPre(msg); // TODO: 적절한 이벤트 생성
    switch (compKind)
    {
    case RepCompKind::NetTransform:
        

        break;
    case RepCompKind::NetHealth:
        //ApplyNetHealthDelta(r, e, fieldMask);
        break;
    default:
        break;
    }
}

Entity NetRecvSystem::CreateEntityFromArchetype(uint32_t archetypeId, const InputCommand& spawnCommand)
{
    return PrefabFactory::Spawn(mWorld, static_cast<PrefabType>(archetypeId), spawnCommand);
}
