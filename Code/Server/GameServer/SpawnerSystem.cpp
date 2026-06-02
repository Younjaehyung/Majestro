#include "pch.h"
#include "SpawnerSystem.h"

#include "World.h"
#include "ServerCore.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameTimer.h"

#include "SpawnerComponent.h"
#include "TransformComponent.h"
#include "GravityComponent.h"
#include "HealthComponent.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"

#include "Prefab.h"

SpawnerSystem::SpawnerSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
}

void SpawnerSystem::Update(float dt)
{
    (void)dt;

    if (!mWorld->HasComponentPool<SpawnerComponent>())
        return;

    // 외부 이벤트로 깨어나야 하는 스포너
    ConsumeExternalTriggers();

    const float now = GetServerTotalTimeSeconds();

    // 살아있는 Spawner 엔티티 전체 순회
    for (Entity e : mWorld->GetEntitiesWithComponent<SpawnerComponent>())
    {
        auto* sp = mWorld->GetComponent<SpawnerComponent>(e);
        if (!sp) continue;
        // 생존 여부 갱신
        PruneDead(sp);

        if (!sp->mActive) continue;

        // Wave 트리거는 외부가 active를 켜준 뒤 시간 기반처럼 동작하게 둔다.
        // OnPlayerEnter도 동일. 활성화된 이후에는 mInterval에 따라 주기 스폰.
        if (sp->mInterval <= 0.0f) continue;
        if (now < sp->mNextSpawnTime) continue;

        if (sp->mMaxTotal >= 0 && sp->mTotalSpawned >= sp->mMaxTotal)
        {
            sp->mActive = false;
            continue;
        }
        if (sp->mMaxAlive >= 0 && static_cast<int32>(sp->mAliveIds.size()) >= sp->mMaxAlive)
        {   // 가득 찼으면 다음 프레임에 다시 체크
            continue;
        }

        Entity spawned = SpawnOne(e, sp);
        if (spawned.IsValid())
        {
            sp->mAliveIds.push_back(spawned.GetID());
            sp->mTotalSpawned++;
        }

        sp->mNextSpawnTime = now + sp->mInterval;
    }
}

void SpawnerSystem::PruneDead(SpawnerComponent* sp)
{
    if (!sp) return;
    auto& alive = sp->mAliveIds;

    alive.erase(
        std::remove_if(alive.begin(), alive.end(),
            [&](EntityID id)
            {
                Entity ent{ id };
                if (!ent.IsValid()) return true;
                if (HealthComponent* h = mWorld->GetComponent<HealthComponent>(ent))
                    return h->IsDead();
                return false;
            }),
        alive.end());
}

Entity SpawnerSystem::SpawnOne(Entity spawnerEntity, SpawnerComponent* sp)
{
    if (!sp) return Entity{};


    Vec3 base(0.0f, 0.0f, 0.0f);
    if (auto* tr = mWorld->GetComponent<TransformComponent>(spawnerEntity))
        base = tr->mLocalPosition;

    // Dummy
    InputCommand dummy{};
    dummy.SessionId = 0;
    dummy.Type = PKT_Type::S2C_PKT_SPAWN;

    Entity spawned = PrefabFactory::Spawn(mWorld, sp->mPrefabType, dummy);
    if (!spawned.IsValid()) return Entity{};

    const Vec3 offset = SampleDiskXZ(sp->mSpawnRadius);
    const Vec3 finalPos = base + offset;
    if (auto* tr = mWorld->GetComponent<TransformComponent>(spawned))
    {
        tr->mLocalPosition = finalPos;
        tr->mWorldMatrix = Matrix::CreateTranslation(finalPos);
    }

    // 스폰 높이 동기화.
    if (auto* grav = mWorld->GetComponent<GravityComponent>(spawned))
    {
        grav->mHight = finalPos.y;
        grav->mGround = finalPos.y;
        grav->mGravity = 0.0f;
        grav->mFalling = false;
        grav->mDropping = false;
        grav->mGroundGraceLeft = 0.0f;
    }

    BroadcastSpawnPacket(spawned, static_cast<uint8>(sp->mPrefabType));
    return spawned;
}

void SpawnerSystem::BroadcastSpawnPacket(Entity spawnedEntity, uint8 prefabTypeRaw)
{
    NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(spawnedEntity);
    if (!netComp) return;

    const PrefabType prefabType = static_cast<PrefabType>(prefabTypeRaw);

    S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(0, netComp->mNetEntityId, prefabType);
    spawnPkt.isLocalPlayer = 0;
    if (EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(spawnedEntity))
        spawnPkt.Type = enemyComp->mEnemyType;

    for (uint32 sessionId : CollectPlayerSessions())
    {
        SendRequest request{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
        request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
        gSendQueue.Push(request);
    }

    // 체력 초기값도 갱신해두면 클라 HUD가 스폰 직후 올바른 값을 본다.
    if (auto em = mWorld->GetEventManager())
    {
        if (HealthComponent* hp = mWorld->GetComponent<HealthComponent>(spawnedEntity))
        {
            EvHealthChanged evt{};
            evt.target    = spawnedEntity;
            evt.currentHp = hp->mCurrentHp;
            evt.maxHp     = hp->mMaxHp;
            em->Enqueue<EvHealthChanged>(evt);
        }
    }
}

std::vector<uint32> SpawnerSystem::CollectPlayerSessions() const
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

void SpawnerSystem::ConsumeExternalTriggers()
{
    auto em = mWorld->GetEventManager();
    if (!em) return;

    // Interactable 볼륨이 소비되면 같은 위치의 OnPlayerEnter 스포너를 깨운다.
    // 연결 규칙: Interactable 엔티티와 Spawner 엔티티가 "같은 Entity"이면 바로 활성화.
    // (복잡한 매핑이 필요하면 외래키 필드를 Interactable에 추가할 것)
    em->Consume<EvInteractableConsumed>([&](const EvInteractableConsumed& ev)
    {
        if (!ev.trigger.IsValid()) return;
        if (SpawnerComponent* sp = mWorld->GetComponent<SpawnerComponent>(ev.trigger))
        {
            if (sp->mTrigger == SpawnerTrigger::OnPlayerEnter)
            {
                sp->mActive = true;
                // 즉시 첫 스폰을 허용
                sp->mNextSpawnTime = GetServerTotalTimeSeconds();
            }
        }
    });
}
