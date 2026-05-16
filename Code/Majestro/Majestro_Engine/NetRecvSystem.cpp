#include "pch.h"
#include "NetRecvSystem.h"
#include <algorithm>
#include <chrono>
#include "EnginePch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "World.h"
#include "Timer.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "BulletComponent.h"
#include "NetIdMap.h"
#include "TransformComponent.h"
#include "NetTransformComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "TagComponent.h"
#include "BoxColliderComponent.h"
#include "NetSendSystem.h"
#include "MovementSystem.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "VfxComponent.h"
#include "ResourceManager.h"
#include "GameRuleComponent.h"

namespace
{
    struct BulletVfxSpec
    {
        const wchar_t* effectName;
        Vec3 scale;
    };

    BulletVfxSpec ResolveBulletVfxSpec(SkillType type)
    {
        switch (type)
        {
        case SkillType::GuitarAttack:
        case SkillType::GuitarAttack_1:
        case SkillType::GuitarAttack_2:
        case SkillType::GuitarAttack_3:
            return { L"VFX_Fanthor_Slash_01", Vec3(12.0f, 12.0f, 12.0f) };

        case SkillType::BaseAttack:
        case SkillType::BaseSkill1:
            return  { L"VFX_Ibanix_Bullet", Vec3(12.0f, 12.0f, 12.0f) };
        case SkillType::HornAttack:
        default:
            return { L"VFX_Ibanix_Bullet", Vec3(2.0f, 2.0f, 2.0f) };
        }
    }
}

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
    reg(PKT_Type::S2C_PKT_COLLISION,         [this](auto& m){ HandleCollision(m); });
    reg(PKT_Type::S2C_PKT_BULLET_ACTIVATE,   [this](auto& m){ HandleBulletActivate(m); });
    reg(PKT_Type::S2C_PKT_BULLET_DEACTIVATE, [this](auto& m){ HandleBulletDeactivate(m); });
    reg(PKT_Type::S2C_PKT_EFFECT_SPAWN,      [this](auto& m){ HandleEffectSpawn(m); });
    reg(PKT_Type::S2C_PKT_HIT_CONFIRM,       [this](auto& m){ HandleHitConfirm(m); });
    reg(PKT_Type::S2C_GAME_START,            [this](auto& m){ HandleGameStart(m); });
    reg(PKT_Type::S2C_SCENE_CHANGE_RESULT,   [this](auto& m){ HandleSceneChangeResult(m); });
	reg(PKT_Type::S2C_PKT_SCENE_STATE,       [this](auto& m) { HandleSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_CONQUEST, [this](auto& m) { HandleConquestSceneState(m); });
	reg(PKT_Type::S2C_PKT_SCENE_ESCORT, [this](auto& m) { HandleEscortSceneState(m); });
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

       
    }
    else if (enemyComp)
    {
        enemyComp->mAnimState = pkt->stateId;
    }

    if (netTransform)
        netTransform->mElapsed = 0.0f;


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

    healthComp->mCurrentHp = pkt->currentHp;
    healthComp->mMaxHp     = pkt->maxHp;

    mWorld->GetEventManager()->Enqueue(EvHealthChanged{
       e, pkt->currentHp, pkt->maxHp
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

    const BulletVfxSpec vfxSpec = ResolveBulletVfxSpec(bulletType);
    VfxComponent* bulletVfx = mWorld->GetComponent<VfxComponent>(bulletEntity);
    if (bulletVfx == nullptr)
        bulletVfx = &mWorld->AddComponent<VfxComponent>(bulletEntity);

    if (bulletVfx)
    {
        bulletVfx->mVfx = RESOURCEMANAGER.Get<Vfx>(vfxSpec.effectName);

       
        bulletVfx->mScale = vfxSpec.scale;
        bulletVfx->mIsLoop = true;
        bulletVfx->mIsPaused = false;
        bulletVfx->mIsPlaying = false;
        bulletVfx->mShouldPlay = (bulletVfx->mVfx != nullptr);
        bulletVfx->mTotalTime = 0.f;
    }

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

void NetRecvSystem::HandleEffectSpawn(const InputCommand& msg)
{
    const S2C_EffectSpawnPacket* pkt = msg.ViewAs<S2C_EffectSpawnPacket>();
    if (!pkt) return;
    /*constexpr uint8 kEffectSpawnReasonFire = 0;
    if (pkt->reason == kEffectSpawnReasonFire) return;*/

    const SkillType effectSkillType = static_cast<SkillType>(pkt->effectType);

    auto isBaseSkill = [](SkillType type)
        {
            return type == SkillType::BaseAttack ||
                type == SkillType::BaseSkill1 ||
                type == SkillType::BaseSkill2;
        };

    auto isStaticImpactSkill = [&](SkillType type)
        {
           
            return isBaseSkill(type) ||
                type == SkillType::GuitarAttack_1 ||
                type == SkillType::GuitarAttack_2 ||
                type == SkillType::GuitarAttack_3 ||
                type == SkillType::HornAttack;
        };

    const wchar_t* effectName = nullptr;
    Vec3 effectScale = Vec3(1.0f);
    bool effectLoop = false;


    TransformComponent impactTransform{};
    switch (pkt->reason)
    {
    case 0:
        if (effectSkillType == SkillType::GuitarAttack)
        {
            effectName = L"VFX_Fanthor_Slash_01";
            impactTransform.mLocalRotationE = Vec3(pkt->rotX, pkt->rotY, pkt->rotZ);
            impactTransform.mLocalPosition = Vec3(pkt->x, pkt->y+100, pkt->z);
            effectScale = Vec3(30.0f);
        }
        else if (effectSkillType == SkillType::GuitarSkill1)
        {
            effectName = L"VFX_Fanthor_Skill_01";
            impactTransform.mLocalRotationE = Vec3(pkt->rotX, pkt->rotY, pkt->rotZ);
            impactTransform.mLocalPosition = Vec3(pkt->x, pkt->y+100, pkt->z);
            effectScale = Vec3(30.0f);
        }
        break;
    case 1:
        if (isBaseSkill(effectSkillType))
        {
            effectName = L"VFX_Ibanix_Attack_Hit_01";
            impactTransform.mLocalRotationE = Vec3(pkt->rotX, pkt->rotY, pkt->rotZ);
            impactTransform.mLocalPosition = Vec3(pkt->x, pkt->y, pkt->z);
            effectScale = Vec3(30.f);
        }
        break;
    case 2:
        if (isStaticImpactSkill(effectSkillType))
        {
  
            effectName = L"VFX_Ibanix_Attack_Hit_01";
            impactTransform.mLocalRotationE = Vec3(pkt->rotX, pkt->rotY, pkt->rotZ);
            impactTransform.mLocalPosition = Vec3(pkt->x, pkt->y, pkt->z);
            effectScale = Vec3(30.f);
        }
        break;
    }


    if (!effectName) return;
    shared_ptr<Vfx> selectedVfx = RESOURCEMANAGER.Get<Vfx>(effectName);
    if (!selectedVfx) return;

    Entity impactVfxEntity = mWorld->CreateEntity();
   
    
    mWorld->AddComponent<TransformComponent>(impactVfxEntity, impactTransform);


    cout << "rot:" << pkt->rotY << endl;
    VfxComponent& impactVfx = mWorld->AddComponent<VfxComponent>(impactVfxEntity);
    impactVfx.mVfx = selectedVfx;
    impactVfx.mScale = effectScale;
    impactVfx.mIsLoop = effectLoop;
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
	gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::FirstGame, L"게임 씬 로딩 중...");
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
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::Lobby, L"로비 씬 로딩 중...");
        break;
    case SceneId::FirstGame:
		gEngine->GetSceneManager().RequestSceneWithLoading(SceneId::FirstGame, L"게임 씬 로딩 중...");
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

	 Entity e = mWorld->GetGameRuleEntity();

	 GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	 if (!gameRuleComp) return;

	 gameRuleComp->mGameTime = pkt->GameTime;
	 gameRuleComp->mPlayerScore = pkt->PlayerScore;

	 if (gameRuleComp->mGamePhase != pkt->GamePhase)
		 mWorld->GetEventManager()->Enqueue(EvGamePhaseChanged{ gameRuleComp->mGamePhase, pkt->GamePhase });

	 gameRuleComp->mGamePhase = pkt->GamePhase;
}

void NetRecvSystem::HandleConquestSceneState(const InputCommand& msg)
{
    const S2C_ConquestPacket* pkt = msg.ViewAs<S2C_ConquestPacket>();
	if (!pkt) return;
	// Conquest 씬 상태 정보 처리 (예: 점령 상태, 팀 점수 등)
    
    Entity e = mWorld->GetGameRuleEntity();

	GameConquestComponent* conquestComp = mWorld->GetComponent<GameConquestComponent>(e);
	if (!conquestComp) return;

	conquestComp->mWaveCheckPoint = pkt->WaveCheckPoint;
	conquestComp->mWave = pkt->Wave;
	conquestComp->mWaveInterval = pkt->WaveInterval;
	conquestComp->mWaveTime = pkt->WaveTime;
	conquestComp->mPlayerNum = pkt->PlayerNum;
	conquestComp->mEnemyNum = pkt->EnemyNum;

}

void NetRecvSystem::HandleEscortSceneState(const InputCommand& msg)
{
    const S2C_EscortPacket* pkt = msg.ViewAs<S2C_EscortPacket>();
    if (!pkt) return;
    // Escort 씬 상태 정보 처리 (예: 호위 대상 HP, 남은 시간 등)
    Entity e = mWorld->GetGameRuleEntity();
    GameEscortComponent* escortComp = mWorld->GetComponent<GameEscortComponent>(e);
    if (!escortComp) return;
    //escortComp->mRouteId = pkt->RouteId;
    escortComp->mEscortProgress = pkt->EscortProgress;
	escortComp->mEscortStage = pkt->EscortStage;
	escortComp->mEscortTime = pkt->EscortTime;
}


void NetRecvSystem::HandleSpawn(const InputCommand& msg)
{

    uint32_t netId = 0;
    uint32_t archetypeId = 0;
	const S2C_SpawnPacekt* spawnPacket = msg.ViewAs<S2C_SpawnPacekt>();
	archetypeId = static_cast<uint32_t>(spawnPacket->prefabType);
	netId = static_cast<uint32_t>(spawnPacket->netEntityId);
	std::cout << "HandleSpawn called with netId: " << netId << " archetypeId: " << archetypeId << std::endl;
    if (mWorld->GetEntityByNetId(netId) == NULL_ENTITY) {
        Entity e = CreateEntityFromArchetype(archetypeId);
        mWorld->NetIdBinding(netId, e);
		std::cout << "HandleSpawn: netId=" << netId << " -> entityId=" << e.GetID() << std::endl;
    }
    else {
        std::cout << "HandleSpawn: netId=" << netId << " 이미 존재, 스킵" << std::endl;
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
    const S2C_SpawnPacekt* pkt = msg.ViewAs<S2C_SpawnPacekt>();
    if (!pkt) return;

    const uint32_t netId = static_cast<uint32_t>(pkt->netEntityId);
    Entity e = mWorld->GetEntityByNetId(netId);
    if (e == NULL_ENTITY) return;

    // TODO: ECS 엔티티 삭제는 별도 명령으로 처리 권장
    // mCmd->DestroyEntity(e);

    mWorld->NetIdUnbinding(netId);
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

Entity NetRecvSystem::CreateEntityFromArchetype(uint32_t archetypeId)
{
    //Entity entity = mWorld->CreateEntity();
    return PrefabFactory::Spawn(mWorld, static_cast<PrefabType>(archetypeId), mInputCommand);
}
