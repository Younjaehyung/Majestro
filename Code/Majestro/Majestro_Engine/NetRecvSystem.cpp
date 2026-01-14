#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "NetIdMap.h"
#include "Prefab.h"

NetRecvSystem::NetRecvSystem(World* world, shared_ptr<NetIdMap>& netIdMap) : System::System(world)
{
	mNetIdMap = netIdMap;
}

NetRecvSystem::~NetRecvSystem()
{
}

void NetRecvSystem::Initialize()
{
	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();

	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		mNetIdMap->Bind(netComp->mNetEntityId, entity);
	}
}

void NetRecvSystem::Update(double deltaTime)
{
	return; // 임시 비활성화
    constexpr int kMaxMsgsPerTick = 256; // 폭주 방지
    int processed = 0;

    while (processed < kMaxMsgsPerTick && gRecvBuffer.Pop(mInputCommand))
    {
        ProcessOne(mInputCommand);
        ++processed;
    }

   /* if (mWorld && mCmd)
        mCmd->Flush(*mWorld);*/
}

void NetRecvSystem::ProcessOne(const InputCommand& msg)
{
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

void NetRecvSystem::HandleSpawn(const InputCommand& msg)
{

    uint32_t netId = 0;
    uint32_t archetypeId = 0;
	const S2C_SpawnPacekt* spawnPacket = msg.ViewAs<S2C_SpawnPacekt>();
	archetypeId = static_cast<uint32_t>(spawnPacket->prefabType);
	netId = static_cast<uint32_t>(spawnPacket->netEntityId);

    Entity e = CreateEntityFromArchetype(archetypeId);

    // netId 바인딩 (중요)
    mNetIdMap->Bind(netId, e);

    // 초기 상태도 같이 온다면 반영
    //NetTransformState nts{};
    //if (r.Read(nts)) {
    //    // [중요] 네트워크 상태 컴포넌트만 갱신
    //    mCmd->SetNetTransform(e, nts);
    //}
}

void NetRecvSystem::HandleDespawn(const InputCommand& msg)
{
    uint32_t netId = 0;
   // if (!r.Read(netId)) return;

    Entity e = mNetIdMap->GetOrInvalid(netId);
    if (e == 0) return;

    // TODO: ECS 엔티티 삭제는 별도 명령으로 처리 권장
    // mCmd->DestroyEntity(e);
    mNetIdMap->Unbind(netId);
}

void NetRecvSystem::HandleReplicationDelta(const InputCommand& msg)
{

    uint32_t netId = 0;
    RepCompKind compKind{};
    uint32_t fieldMask = 0;

   /* if (!r.Read(netId)) return;
    if (!r.Read(compKind)) return;
    if (!r.Read(fieldMask)) return;*/

    Entity e = mNetIdMap->GetOrInvalid(netId);
    if (e == 0) {
        // 아직 Spawn이 안 왔거나, 관심영역 늦게 들어온 케이스
        // 실전에서는 여기서 "Spawn 요청" 또는 "임시 보류" 전략을 둠
        return;
    }

    switch (compKind)
    {
    case RepCompKind::NetTransform:
        //ApplyNetTransformDelta(r, e, fieldMask);
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
    Entity entity = mWorld->CreateEntity();
	PrefabFactory::Spawn(mWorld, static_cast<PrefabType>(archetypeId), mInputCommand);

    return entity;
}
