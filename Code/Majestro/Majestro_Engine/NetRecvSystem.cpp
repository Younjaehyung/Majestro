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
#include "BoxColliderComponent.h"
#include "NetSendSystem.h"
#include "MovementSystem.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "VfxComponent.h"
#include "ResourceManager.h"

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
    if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
    if (entities.empty())return;
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
        mWorld->NetIdBinding(netComp->mNetEntityId, entity);
	}
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

    if (msg.Type == PKT_Type::S2C_PKT_SPAWN) {
		std::cout << "Spawn Packet Received in NetRecvSystem" << std::endl;
        HandleSpawn(msg);
        return;
    }
    else if (msg.Type == PKT_Type::S2C_SCENE_CHANGE_RESULT) {
        HandleSceneChangeResult(msg);
    }
    else if (msg.Type == PKT_Type::S2C_GAME_START) {
        cout << "GameStart" << endl;
        gEngine->GetSceneManager().SetLoadingMessage(L"게임 씬 로딩 중...");
        gEngine->GetSceneManager().QueueLoadingScene(L"게임 씬 로딩 중...", LoadingVisualType::GameStart);
        gEngine->GetSceneManager().QueueLoadScene(L"Game");
        return;
    }
    else if (msg.Type == PKT_Type::S2C_PKT_MOVE) {
        const S2C_MovePacket* movePacket = msg.ViewAs<S2C_MovePacket>();
        //std::cout << "State Packet Received in NetRecvSystem for Entity ID: " << statePacket->netEntityId << " with State ID: " << static_cast<int>(statePacket->stateId) << std::endl;

		// msg netity id로 엔티티 찾기
		Entity e = mWorld->GetEntityByNetId(movePacket->netEntityId);

        TransformComponent* comp =  mWorld->GetComponent<TransformComponent>(e);
        NetTransformComponent* netcomp =  mWorld->GetComponent<NetTransformComponent>(e);
		if(comp == nullptr || netcomp == nullptr ) return;

		// 오래된 UDP 패킷 무시: out-of-order 도착한 패킷은 버림
		if (netcomp->mLastSequence != 0 &&
			!NetTransformComponent::IsNewer(movePacket->Sequence, netcomp->mLastSequence))
			return;

		netcomp->mTargetPosition.x = movePacket->x;
		netcomp->mTargetPosition.y = movePacket->y;
		netcomp->mTargetPosition.z = movePacket->z;
		netcomp->mTargetRotation.y = movePacket->yaw;
		netcomp->mTargetRotation.x = movePacket->pitch;
		netcomp->mTargetRotationQ.x = movePacket->rx;
		netcomp->mTargetRotationQ.y = movePacket->ry;
		netcomp->mTargetRotationQ.z = movePacket->rz;
		netcomp->mTargetRotationQ.w = movePacket->rw;
		netcomp->mLastSequence = movePacket->Sequence;
		netcomp->mLastUpdateTime = TIMER.GetTotalTime();
		netcomp->mVelocity = Vec3(movePacket->vx, movePacket->vy, movePacket->vz);
        netcomp->mHasTarget = true;
		/*std::cout << "Move Packet Processed for Entity: " <<" "<<
			comp->mWorldPosition.x << ", " << comp->mWorldPosition.y << ", " << comp->mWorldPosition.z << std::endl;*/
        return;
    }
    else if (msg.Type == PKT_Type::S2C_PKT_STATE) {
      const S2C_StatePacket* statePacket = msg.ViewAs<S2C_StatePacket>();
	   // msg netity id로 엔티티 찾기
      Entity e = mWorld->GetEntityByNetId(statePacket->netEntityId);
      MainPlayerComponent* playercomp = mWorld->GetComponent<MainPlayerComponent>(e);
      NetEntityComponent* comp = mWorld->GetComponent<NetEntityComponent>(e);
      NetTransformComponent* netTransform = mWorld->GetComponent<NetTransformComponent>(e);
      if (comp == nullptr || playercomp == nullptr) return;

      playercomp->mStatePacket = statePacket->stateId;
      playercomp->mLowerStatePacket = statePacket->lowerStateId;
	  netTransform->mElapsed = 0.0f;
      /*switch (statePacket->stateId)
      {
         case S_Idle:
             playercomp->mFsm.ChangeState(playercomp, IdleState::Instance());
			 break;
        case S_Walk:
			playercomp->mFsm.ChangeState(playercomp, WalkState::Instance());
			std::cout << "State Changed to Walk for Entity: " << e.GetID() << std::endl;
            break;
        case S_Run:
			playercomp->mFsm.ChangeState(playercomp, DashState::Instance());
            break;
		case S_Jump:
            playercomp->mFsm.ChangeState(playercomp, JumpState::Instance());
            break;
        default:
			break;

      }*/
      
      return;
    }
    else if (msg.Type == PKT_Type::S2C_PKT_HEALTH) {
        const S2C_HealthPacket* healthPacket = msg.ViewAs<S2C_HealthPacket>();
        Entity e = mWorld->GetEntityByNetId(healthPacket->netEntityId);
        HealthComponent* healthComp = mWorld->GetComponent<HealthComponent>(e);
        if (healthComp == nullptr) return;

        const int32 beforeHp = healthComp->mCurrentHp;
        const int32 beforeMaxHp = healthComp->mMaxHp;

        healthComp->mCurrentHp = healthPacket->currentHp;
        healthComp->mMaxHp = healthPacket->maxHp;

        std::cout << "[Client][S2C_PKT_HEALTH] netEntityId=" << healthPacket->netEntityId
            << " hp=" << beforeHp << "/" << beforeMaxHp
            << " -> " << healthComp->mCurrentHp << "/" << healthComp->mMaxHp
            << std::endl;
        return;
    }
    else if (msg.Type == PKT_Type::S2C_PKT_ARMOR) {
        const S2C_ArmorPacket* armorPacket = msg.ViewAs<S2C_ArmorPacket>();
        Entity e = mWorld->GetEntityByNetId(armorPacket->netEntityId);
        ArmorComponent* armorComp = mWorld->GetComponent<ArmorComponent>(e);
        if (armorComp == nullptr) return;

        const int32 beforeArmor = armorComp->mCurrentArmor;
        const int32 beforeMaxArmor = armorComp->mMaxArmor;

        armorComp->mCurrentArmor = armorPacket->currentArmor;
        armorComp->mMaxArmor = armorPacket->maxArmor;

        std::cout << "[Client][S2C_PKT_ARMOR] netEntityId=" << armorPacket->netEntityId
            << " armor=" << beforeArmor << "/" << beforeMaxArmor
            << " -> " << armorComp->mCurrentArmor << "/" << armorComp->mMaxArmor
            << std::endl;
        return;
    }
    else if(msg.Type == PKT_Type::S2C_PKT_COLLISION) {
		const S2C_CollisionPacket* collisionPacket = msg.ViewAs<S2C_CollisionPacket>();
		// msg netity id로 엔티티 찾기
		Entity e = mWorld->GetEntityByNetId(collisionPacket->netEntityId);
		NetEntityComponent* comp = mWorld->GetComponent<NetEntityComponent>(e);
		if (comp == nullptr) return;
		BoxColliderComponent* boxComp = mWorld->GetComponent<BoxColliderComponent>(e);
		if (boxComp == nullptr) return;
		boxComp->bIsColliding = collisionPacket->bIsColliding;
		//std::cout << "Collision Packet Processed for Entity ID: " << collisionPacket->netEntityId << " Collision State: " << boxComp->bIsColliding << std::endl;
        return;
	}
    else if (msg.Type == PKT_Type::S2C_PKT_BULLET_ACTIVATE) {
        const S2C_BulletActivatePacket* bulletPacket = msg.ViewAs<S2C_BulletActivatePacket>();
        if (bulletPacket == nullptr)
            return;

        Entity bulletEntity = mWorld->GetEntityByNetId(bulletPacket->bulletNetEntityId);
        if (bulletEntity == NULL_ENTITY)
        {
            std::cout << "Bullet Activate Packet Received but entity not found - bullet: " << bulletPacket->bulletNetEntityId << std::endl;
            return;
        }

        BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
        TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
        if (bulletComp == nullptr || bulletTransform == nullptr)
            return;

        Vec3 spawnPosition{ bulletPacket->x, bulletPacket->y, bulletPacket->z };
        Vec3 direction{ bulletPacket->dirX, bulletPacket->dirY, bulletPacket->dirZ };
        if (direction.LengthSquared() <= 0.0001f)
            direction = Vec3::Forward;
        direction.Normalize();

        // 보정: 패킷이 네트워크/큐 대기 후 도착했다면, 그 시간만큼 탄환 위치를 앞당겨 시작한다.
        // clock drift에 대비해 과도 보정은 상한을 둔다.
        const double nowSec = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const double transitSec = (std::max)(0.0, nowSec - bulletPacket->SendTime);
        const float compensationSec = static_cast<float>((std::min)(transitSec, 0.25));
        spawnPosition += direction * bulletPacket->speed * compensationSec;

        bulletTransform->mLocalPosition = spawnPosition;
        bulletTransform->mWorldPosition = spawnPosition;

        const uint16 generation = static_cast<uint16>(bulletComp->mGeneration + 1);
        BulletType bulletType = static_cast<BulletType>(bulletPacket->bulletType);
        if (bulletType >= BulletType::Max)
            bulletType = BulletType::Default;

        bulletComp->Activate(
            bulletType,
            bulletPacket->ownerNetEntityId,
            static_cast<uint32>(bulletPacket->bulletNetEntityId),
            generation,
            spawnPosition,
            direction,
            bulletPacket->speed,
            bulletComp->mLifeTime,
            bulletComp->mDamage);

        bulletComp->mElapsedTime = (std::min)(compensationSec, bulletComp->mLifeTime);
        if (auto movementSystem = mWorld->GetSystemManager()->GetSystem<MovementSystem>())
            movementSystem->RegisterActiveBullet(bulletEntity);
        /*std::cout << "type: " << (int)bulletType << std::endl;

        std::cout << "Bullet Activate Packet Received - owner: " << bulletPacket->ownerNetEntityId
            << ", bullet: " << bulletPacket->bulletNetEntityId
            << ", pos(" << bulletPacket->x << ", " << bulletPacket->y << ", " << bulletPacket->z << ")" << std::endl;*/
        return;
    }
    else if (msg.Type == PKT_Type::S2C_PKT_BULLET_DEACTIVATE) {
        const S2C_BulletDeactivatePacket* bulletPacket = msg.ViewAs<S2C_BulletDeactivatePacket>();
        if (bulletPacket == nullptr)
            return;

        Entity bulletEntity = mWorld->GetEntityByNetId(bulletPacket->bulletNetEntityId);
        if (bulletEntity == NULL_ENTITY)
            return;

        BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
        TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
        if (bulletComp == nullptr || bulletTransform == nullptr)
            return;

        bulletComp->Deactivate();
        bulletTransform->mMovingVector = Vec3::Zero;

        if (/*bulletPacket->hasImpact*/true)
        {
            Entity impactVfxEntity = mWorld->CreateEntity();
            TransformComponent impactTransform{};
            impactTransform.mLocalPosition = Vec3(bulletPacket->impactX, bulletPacket->impactY, bulletPacket->impactZ);
            mWorld->AddComponent<TransformComponent>(impactVfxEntity, impactTransform);

            VfxComponent& impactVfx = mWorld->AddComponent<VfxComponent>(impactVfxEntity);
            //cout << "bullet type:" << (int)bulletComp->mType << endl;
            if (bulletComp->mType == BulletType::BaseAttack) {
                cout << "base attack" << endl;
                
            }
            impactVfx.mVfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Ibanix_Hit_01");
            impactVfx.mScale = 10.f;
            impactVfx.mIsLoop = true;
        }

        if (auto movementSystem = mWorld->GetSystemManager()->GetSystem<MovementSystem>())
            movementSystem->UnregisterActiveBullet(bulletEntity);
        return;
        }


    switch (msg.Kind)
    {
    case MsgKind::ReplicationDelta:
        HandleReplicationDelta(msg);
        break;
    case MsgKind::Spawn:
        HandleSpawn(msg);
        break;
    case MsgKind::Despawn:
        HandleDespawn(msg);
        break;
    default:
        break;
    }
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
        gEngine->GetSceneManager().QueueLoadScene(L"Lobby");
        break;
    case SceneId::Game:
        gEngine->GetSceneManager().SetLoadingMessage(L"게임 씬 로딩 중...");
        gEngine->GetSceneManager().QueueLoadingScene(L"게임 씬 로딩 중...", LoadingVisualType::GameStart);
        gEngine->GetSceneManager().QueueLoadScene(L"Game");
        gEngine->GetSceneManager().QueueGameStartAfterLoad();
        break;
    default:
        break;
    }
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
		std::cout << "Entity created with ID: " << e.GetID() << std::endl;
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
    uint32_t netId = 0;
   // if (!r.Read(netId)) return;

    Entity e = mWorld->GetEntityByNetId(netId);
    if (e == 0) return;

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
