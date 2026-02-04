#include "pch.h"
#include "NetRecvSystem.h"
#include "EnginePch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "NetIdMap.h"
#include "TransformComponent.h"
#include "NetTransformComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include "BoxColliderComponent.h"
#include "NetSendSystem.h"

NetRecvSystem::NetRecvSystem(World* world, EventManager* event, shared_ptr<NetIdMap>& netIdMap)
	: System::System(world, event)
{
	mNetIdMap = netIdMap;
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

void NetRecvSystem::Update(double deltaTime)
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
        gEngine->GetSceneManager().LoadScene(L"Game");
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
		netcomp->mTargetPosition.x = movePacket->x;
		netcomp->mTargetPosition.y = movePacket->y;
		netcomp->mTargetPosition.z = movePacket->z;
		netcomp->mTargetRotation.y = movePacket->yaw;
		netcomp->mTargetRotation.x = movePacket->pitch;
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
