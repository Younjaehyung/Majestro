#include "pch.h"
#include "World.h"
#include "ResourceManager.h"
#include "CollisionSystem.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "TagComponent.h"
#include "PhysicsWorld.h"
#include "BulletComponent.h"
#include "NetEntityComponent.h"
#include "GameEvents.h"

#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"

#include <cmath>

namespace
{
    bool NormalizeOBBOrientation(BoundingOrientedBox& obb)
    {
        const XMVECTOR orientation = XMLoadFloat4(&obb.Orientation);
        const float length = XMVectorGetX(XMQuaternionLength(orientation));
        if (!std::isfinite(length) || length <= 1e-6f)
        {
            // 수정 내용
            // DirectXCollision Intersects 는 OBB Orientation 이 단위 쿼터니언이어야 한다.
            // 로딩 데이터나 이전 변환 경로에서 깨진 값이 들어오면 충돌 검사 직전에 identity 로 보정한다.
            obb.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            return false;
        }

        XMFLOAT4 normalized{};
        XMStoreFloat4(&normalized, XMQuaternionNormalize(orientation));
        obb.Orientation = normalized;
        return std::abs(length - 1.0f) <= 1e-4f;
    }

    void NormalizeCollisionOBB(BoxColliderComponent* collider)
    {
        if (!collider)
            return;

        NormalizeOBBOrientation(collider->mLocalOBB);
        NormalizeOBBOrientation(collider->mWorldOBB);
    }

    bool IsDeadEnemy(World* world, Entity entity)
    {
        if (!world || !entity.IsValid())
            return false;

        if (!world->HasComponent<EnemyComponent>(entity))
            return false;

        const HealthComponent* health = world->GetComponent<HealthComponent>(entity);
        return health && health->mCurrentHp <= 0;
    }
}

 CollisionSystem::CollisionSystem(World* world) : System(world)
{
     mPhysicsWorld = mWorld->GetPhysicsWorld();
}


 void CollisionSystem::Initialize()
 {
     // Fix: CollisionSystem is registered after scene setup, so refresh static BVH here once.
     if (mPhysicsWorld)
         mPhysicsWorld->SyncStaticBVHIfNeeded();
     
 }

void CollisionSystem::Update(float dt)
{
    if (false == mWorld->HasComponentPool<BoxColliderComponent>())return;
    if (false == mWorld->HasComponentPool<TransformComponent>())return;


    Movable2Movable(dt);
    Movable2Static(dt);
    Bullet2MovableCCD(dt);
    Bullet2StaticCCD(dt);
}

void CollisionSystem::Movable2Movable(float deltaTime)
{
    if (false == mWorld->HasComponentPool<MovableComponent>())return;

    auto entities = mWorld->GetEntitiesWithComponents<MovableComponent, TransformComponent, BoxColliderComponent>();

    std::vector<Entity> activeEntities;
    std::vector<BoxColliderComponent*> colliders;
    std::vector<float> minX;
    std::vector<float> maxX;
    std::vector<float> minZ;
    std::vector<float> maxZ;

    activeEntities.reserve(entities.size());
    colliders.reserve(entities.size());
    minX.reserve(entities.size());
    maxX.reserve(entities.size());
    minZ.reserve(entities.size());
    maxZ.reserve(entities.size());

    float worldMinX = (std::numeric_limits<float>::max)();
    float worldMaxX = std::numeric_limits<float>::lowest();
    float worldMinZ = (std::numeric_limits<float>::max)();
    float worldMaxZ = std::numeric_limits<float>::lowest();
    float maxExtent = 0.0f;

    // (B) 월드 OBB 갱신 + SoA 데이터 채우기
    for (auto e : entities)
    {
        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (col) col->bIsColliding = false;
        if (!tr || !col) continue;
        if (IsDeadEnemy(mWorld, e)) continue;

        PhysicsWorld::UpdateWorldOBB(tr, col);

        XMFLOAT3 corners[8];
        col->mWorldOBB.GetCorners(corners);

        float localMinX = corners[0].x;
        float localMaxX = corners[0].x;
        float localMinZ = corners[0].z;
        float localMaxZ = corners[0].z;
        float localMaxExtent = 0.0f;

        for (const auto& corner : corners)
        {
            localMinX = (std::min)(localMinX, corner.x);
            localMaxX = (std::max)(localMaxX, corner.x);
            localMinZ = (std::min)(localMinZ, corner.z);
            localMaxZ = (std::max)(localMaxZ, corner.z);
            localMaxExtent = (std::max)(localMaxExtent, (std::max)(std::abs(corner.x - col->mWorldOBB.Center.x), std::abs(corner.z - col->mWorldOBB.Center.z)));
        }

        worldMinX = (std::min)(worldMinX, localMinX);
        worldMaxX = (std::max)(worldMaxX, localMaxX);
        worldMinZ = (std::min)(worldMinZ, localMinZ);
        worldMaxZ = (std::max)(worldMaxZ, localMaxZ);
        maxExtent = (std::max)(maxExtent, localMaxExtent);

        activeEntities.push_back(e);
        colliders.push_back(col);
        minX.push_back(localMinX);
        maxX.push_back(localMaxX);
        minZ.push_back(localMinZ);
        maxZ.push_back(localMaxZ);
    }

   
        if (activeEntities.size() < 2)
            return;

        auto runSAP = [&]()
        {
                std::vector<uint32_t> order(activeEntities.size());
                for (uint32_t i = 0; i < static_cast<uint32_t>(order.size()); ++i)
            {
                    order[i] = i;
                }

                std::sort(order.begin(), order.end(), [&](uint32_t lhs, uint32_t rhs)
                {
                        if (minX[lhs] == minX[rhs])
                            return maxX[lhs] < maxX[rhs];
                        return minX[lhs] < minX[rhs];
                    });

                std::vector<uint32_t> activeList;
                activeList.reserve(order.size());

                for (uint32_t currentIndex : order)
                {
                    const float currentMinX = minX[currentIndex];

                    activeList.erase(
                        std::remove_if(activeList.begin(), activeList.end(), [&](uint32_t otherIndex)
                            {
                                return maxX[otherIndex] < currentMinX;
                            }),
                        activeList.end());

                    auto* colA = colliders[currentIndex];
                    if (!colA)
                    {
                        activeList.push_back(currentIndex);
                        continue;
                    }

                    for (uint32_t otherIndex : activeList)
                    {
                        if (maxZ[currentIndex] < minZ[otherIndex] || maxZ[otherIndex] < minZ[currentIndex])
                            continue;

                        auto* colB = colliders[otherIndex];
                        if (!colB) continue;

                    if (colA->mWorldOBB.Intersects(colB->mWorldOBB))
                    {
                        colA->bIsColliding = true;
                        colB->bIsColliding = true;

                        AvoidCollisionByMovementState(
                            mWorld,
                            activeEntities[currentIndex],
                            activeEntities[otherIndex],
                            colA,
                            colB,
                            deltaTime);
                    }
                }
                    activeList.push_back(currentIndex);
            }
        };

   
    const float cellSize = (std::max)(10.0f, maxExtent * 2.0f);
    const float width = worldMaxX - worldMinX;
    const float depth = worldMaxZ - worldMinZ;
    const size_t cellsX = (std::max<size_t>)(1, static_cast<size_t>(std::floor(width / cellSize)) + 1);
    const size_t cellsZ = (std::max<size_t>)(1, static_cast<size_t>(std::floor(depth / cellSize)) + 1);
    const size_t cellCount = cellsX * cellsZ;
    constexpr size_t kMaxGridCells = 1'000'000;

    if (cellCount > kMaxGridCells)
    {
        runSAP();
        return;
    }

    // (C) SoA + Prefix Sum 기반 셀 빌드
    std::vector<uint32_t> counts(cellCount, 0);
    const float invCellSize = 1.0f / cellSize;

    auto clampIndex = [](int value, int minValue, int maxValue)
        {
            return (std::max)(minValue, (std::min)(value, maxValue));
        };
        
    for (size_t i = 0; i < activeEntities.size(); ++i)
    {
        int minCellX = static_cast<int>(std::floor((minX[i] - worldMinX) * invCellSize));
        int maxCellX = static_cast<int>(std::floor((maxX[i] - worldMinX) * invCellSize));
        int minCellZ = static_cast<int>(std::floor((minZ[i] - worldMinZ) * invCellSize));
        int maxCellZ = static_cast<int>(std::floor((maxZ[i] - worldMinZ) * invCellSize));

        minCellX = clampIndex(minCellX, 0, static_cast<int>(cellsX) - 1);
        maxCellX = clampIndex(maxCellX, 0, static_cast<int>(cellsX) - 1);
        minCellZ = clampIndex(minCellZ, 0, static_cast<int>(cellsZ) - 1);
        maxCellZ = clampIndex(maxCellZ, 0, static_cast<int>(cellsZ) - 1);

        for (int z = minCellZ; z <= maxCellZ; ++z)
        {
            const size_t base = static_cast<size_t>(z) * cellsX;
            for (int x = minCellX; x <= maxCellX; ++x)
            {
                counts[base + static_cast<size_t>(x)]++;
            }
        }
    }

    std::vector<uint32_t> offsets(cellCount + 1, 0);
    for (size_t i = 0; i < cellCount; ++i)
    {
        offsets[i + 1] = offsets[i] + counts[i];
    }

    std::vector<uint32_t> cursor = offsets;
    std::vector<uint32_t> indices(offsets.back());

    for (size_t i = 0; i < activeEntities.size(); ++i)
    {
        int minCellX = static_cast<int>(std::floor((minX[i] - worldMinX) * invCellSize));
        int maxCellX = static_cast<int>(std::floor((maxX[i] - worldMinX) * invCellSize));
        int minCellZ = static_cast<int>(std::floor((minZ[i] - worldMinZ) * invCellSize));
        int maxCellZ = static_cast<int>(std::floor((maxZ[i] - worldMinZ) * invCellSize));

        minCellX = clampIndex(minCellX, 0, static_cast<int>(cellsX) - 1);
        maxCellX = clampIndex(maxCellX, 0, static_cast<int>(cellsX) - 1);
        minCellZ = clampIndex(minCellZ, 0, static_cast<int>(cellsZ) - 1);
        maxCellZ = clampIndex(maxCellZ, 0, static_cast<int>(cellsZ) - 1);

        for (int z = minCellZ; z <= maxCellZ; ++z)
        {
            const size_t base = static_cast<size_t>(z) * cellsX;
            for (int x = minCellX; x <= maxCellX; ++x)
            {
                const size_t cellIndex = base + static_cast<size_t>(x);
                indices[cursor[cellIndex]++] = static_cast<uint32_t>(i);
            }
        }
    }

    
    checkedPairs.clear();

    
        for (size_t cell = 0; cell < cellCount; ++cell)
        {
            const uint32_t start = offsets[cell];
            const uint32_t end = offsets[cell + 1];
            for (uint32_t a = start; a < end; ++a)
            {
                const uint32_t idxA = indices[a];
                auto* colA = colliders[idxA];
                for (uint32_t b = a + 1; b < end; ++b)
                {
                    const uint32_t idxB = indices[b];
                    if (idxA == idxB) continue;

                    const EntityID idA = activeEntities[idxA].GetID();
                    const EntityID idB = activeEntities[idxB].GetID();
                    const uint64_t key = idA < idB
                        ? (static_cast<uint64_t>(idA) << 32) | idB
                        : (static_cast<uint64_t>(idB) << 32) | idA;

                    if (!checkedPairs.insert(key).second)
                        continue;

                    auto* colB = colliders[idxB];
                    if (!colA || !colB) continue;

                    if (colA->mWorldOBB.Intersects(colB->mWorldOBB))
                    {
                        colA->bIsColliding = true;
                        colB->bIsColliding = true;

                        AvoidCollisionByMovementState(
                            mWorld,
                            activeEntities[idxA],
                            activeEntities[idxB],
                            colA,
                            colB, 
                            deltaTime);
                    }
                }
            }
        }
}

void CollisionSystem::Movable2Static(float deltaTime)
{
    if (false == mWorld->HasComponentPool<MovableComponent>())return;
    if (false == mWorld->HasComponentPool<StaticComponent>())return;
    if (!mPhysicsWorld)return;

    // Fix: StaticComponent data may be attached after World::Initialize, so keep the BVH in sync before querying.
    mPhysicsWorld->SyncStaticBVHIfNeeded();
    auto& staticProxies = mPhysicsWorld->GetStaticProxies();
    if (staticProxies.empty())
        return;

    // Fix: static collision flags must be reset per frame before Movable2Static marks current hits.
    for (auto& st : staticProxies)
    {
        // 수정 내용
        // StaticProxy 의 ColliderBox 포인터는 ComponentPool vector 재할당 후 무효화될 수 있다.
        // Entity 로 현재 컴포넌트 주소를 다시 얻어서 힙 손상과 랜덤 assertion 을 방지한다.
        BoxColliderComponent* staticCollider = mWorld->GetComponent<BoxColliderComponent>(st.ColliderEntity);
        st.ColliderBox = staticCollider;
        if (staticCollider)
            staticCollider->bIsColliding = false;
    }

    auto dynamicEntities = mWorld->GetEntitiesWithComponents<MovableComponent, TransformComponent, BoxColliderComponent>();
    


    mDynamicObjects.clear();

   // mDynamicObjects.reserve(dynamicEntities.size());

  
    for (auto e : dynamicEntities)
    {
        if (IsDeadEnemy(mWorld, e))
            continue;

        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (!tr || !col)
            continue;

        PhysicsWorld::UpdateWorldOBB(tr, col);
        // 수정 내용
        // 정적 충돌 검사에서 DirectXCollision assertion 이 나지 않도록 동적 OBB 도 검사 전 단위 쿼터니언으로 맞춘다.
        NormalizeCollisionOBB(col);
        const AABB2D bounds = PhysicsWorld::BuildAABBFromOBB(col->mWorldOBB);

        mDynamicObjects.push_back(DynamicProxy{ e, col, bounds });
    }

    if (/*staticObjects.empty() ||*/ mDynamicObjects.empty())
        return;

    

    std::vector<int> candidates;
    for (auto& dyn : mDynamicObjects)
    {
        candidates.clear();
        mPhysicsWorld->QueryStaticBVH(dyn.bounds, candidates);

        for (int candidateIndex : candidates)
        {
            if (candidateIndex < 0 || candidateIndex >= static_cast<int>(staticProxies.size()))
                continue;

            auto& st = mPhysicsWorld->GetStaticProxy(candidateIndex);
            // 수정 내용
            // ComponentPool 이 재할당되면 캐시된 정적 콜라이더 포인터가 댕글링될 수 있으므로
            // 충돌 검사 직전에 Entity 로 최신 포인터를 다시 조회한다.
            BoxColliderComponent* staticCollider = mWorld->GetComponent<BoxColliderComponent>(st.ColliderEntity);
            st.ColliderBox = staticCollider;
            if (!staticCollider)
                continue;

            // 수정 내용
            // LoadCollisionJson 로 들어온 정적 OBB 는 JSON basis 와 월드 행렬 변환 영향을 받는다.
            // Intersects 직전에 한번 더 단위 쿼터니언으로 보정해서 B_quat assertion 을 방지한다.
            NormalizeCollisionOBB(dyn.collider);
            NormalizeCollisionOBB(staticCollider);

            if (!dyn.collider->mWorldOBB.Intersects(staticCollider->mWorldOBB))
                continue;

            dyn.collider->bIsColliding = true;
            staticCollider->bIsColliding = true;

            AvoidCollisionByMovementState(
                mWorld,
                dyn.entity,
                st.ColliderEntity,
                dyn.collider,
                staticCollider,
                deltaTime);
        }
    }
}

void CollisionSystem::Bullet2MovableCCD(float deltaTime)
{
    (void)deltaTime;

    if (false == mWorld->HasComponentPool<BulletComponent>()) return;
    if (false == mWorld->HasComponentPool<TransformComponent>()) return;
    if (false == mWorld->HasComponentPool<MovableComponent>()) return;
    if (false == mWorld->HasComponentPool<BoxColliderComponent>()) return;

    auto& activeBulletEntityIds = mWorld->GetActiveBulletEntityIds();
    auto dynamicEntities = mWorld->GetEntitiesWithComponents<MovableComponent, TransformComponent, BoxColliderComponent>();

    for (size_t i = 0; i < activeBulletEntityIds.size();)
    {
        Entity bulletEntity{ activeBulletEntityIds[i] };
        BulletComponent* bullet = mWorld->GetComponent<BulletComponent>(bulletEntity);
        TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
        if (!bullet || !bulletTransform || !bullet->mIsActive)
        {
            mWorld->UnregisterActiveBullet(bulletEntity);
            continue;
        }

        const Vec3 movement = bulletTransform->mMovingVector;
        if (movement.LengthSquared() <= 1e-8f)
        {
            ++i;
            continue;
        }

        Vec3 direction = movement;
        direction.Normalize();

        const float segmentLength = movement.Length();
        const Vec3 endPosition = bulletTransform->mLocalPosition;
        const Vec3 startPosition = endPosition - movement;

        float bulletRadius = 0.1f;
        BoxColliderComponent* bulletCollider = mWorld->GetComponent<BoxColliderComponent>(bulletEntity);
        if (bulletCollider)
        {
            PhysicsWorld::UpdateWorldOBB(bulletTransform, bulletCollider);
            bulletRadius = (std::max)(bulletRadius,
                (std::max)(bulletCollider->mWorldOBB.Extents.x, bulletCollider->mWorldOBB.Extents.z));
        }
        else
        {
            const float scaleRadius = (std::max)(0.1f, (std::max)(bulletTransform->mLocalScale.x, bulletTransform->mLocalScale.z) * 0.5f);
            bulletRadius = (std::max)(bulletRadius, scaleRadius);
        }

        struct BulletHitCandidate
        {
            Entity Target{};
            BoxColliderComponent* Collider = nullptr;
            float Distance = (std::numeric_limits<float>::max)();
        };

        std::vector<BulletHitCandidate> hitCandidates;
        hitCandidates.reserve(dynamicEntities.size());
        float nearestHitDistance = (std::numeric_limits<float>::max)();
        const Entity instigator = mWorld->GetEntityByNetId(bullet->mOwnerNetId);
        const auto canDamageTarget = [&](Entity attacker, Entity target) -> bool
            {
                if (!attacker.IsValid() || !target.IsValid())
                    return false;

                const bool attackerIsPlayer = mWorld->HasComponent<MainPlayerComponent>(attacker);
                const bool attackerIsEnemy = mWorld->HasComponent<EnemyComponent>(attacker);
                if (!attackerIsPlayer && !attackerIsEnemy)
                    return false;

                const bool targetIsPlayer = mWorld->HasComponent<MainPlayerComponent>(target);
                const bool targetIsEnemy = mWorld->HasComponent<EnemyComponent>(target);

                if (attackerIsPlayer)
                    return targetIsEnemy;

                if (attackerIsEnemy)
                    return targetIsPlayer;

                return false;
            };

        for (Entity targetEntity : dynamicEntities)
        {
            if (targetEntity == bulletEntity)
                continue;
            if (IsDeadEnemy(mWorld, targetEntity))
                continue;

            if (mWorld->HasComponent<BulletComponent>(targetEntity))
                continue;

            NetEntityComponent* targetNetComp = mWorld->GetComponent<NetEntityComponent>(targetEntity);
            if (targetNetComp && bullet->mOwnerNetId != 0 && targetNetComp->mNetEntityId == bullet->mOwnerNetId)
                continue;
            if (instigator.IsValid() && !canDamageTarget(instigator, targetEntity))
                continue;

            TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(targetEntity);
            BoxColliderComponent* targetCollider = mWorld->GetComponent<BoxColliderComponent>(targetEntity);
            if (!targetTransform || !targetCollider)
                continue;

            PhysicsWorld::UpdateWorldOBB(targetTransform, targetCollider);

            BoundingOrientedBox expanded = targetCollider->mWorldOBB;
            expanded.Extents.x += bulletRadius;
            expanded.Extents.y += bulletRadius;
            expanded.Extents.z += bulletRadius;

            float candidateDistance = (std::numeric_limits<float>::max)();
            bool candidateHit = false;

            if (expanded.Contains(startPosition) != ContainmentType::DISJOINT)
            {
                candidateDistance = 0.0f;
                candidateHit = true;
            }
            else if (expanded.Intersects(startPosition, direction, candidateDistance) &&
                candidateDistance >= 0.0f && candidateDistance <= segmentLength)
            {
                candidateHit = true;
            }

            if (!candidateHit)
                continue;

            nearestHitDistance = (std::min)(nearestHitDistance, candidateDistance);
            hitCandidates.push_back(BulletHitCandidate{ targetEntity, targetCollider, candidateDistance });
        }

        if (hitCandidates.empty())
        {
            ++i;
            continue;
        }

        const bool shouldPenetrate = bullet->mbPenetrates;
        if (!shouldPenetrate)
        {
            bulletTransform->mLocalPosition = startPosition + direction * nearestHitDistance;
            bulletTransform->mMovingVector = Vec3::Zero;
        }

        auto applyKnockback = [&](Entity target)
        {
            if (bullet->mKnockbackDistance <= 0.0f)
                return;

                TransformComponent* hitTransform = mWorld->GetComponent<TransformComponent>(target);
            if (hitTransform)
            {
                Vec3 knockbackDirection = direction;
                knockbackDirection.y = 0.0f;

                if (knockbackDirection.LengthSquared() <= 1e-6f)
                {
                    knockbackDirection = hitTransform->mLocalPosition - bulletTransform->mLocalPosition;
                    knockbackDirection.y = 0.0f;
                }

                if (knockbackDirection.LengthSquared() > 1e-6f)
                {
                    knockbackDirection.Normalize();
                    const Vec3 knockbackVector = knockbackDirection * bullet->mKnockbackDistance;
                    hitTransform->mLocalPosition += knockbackVector;
                    hitTransform->mMovingVector += knockbackVector;
                }
            }
            };

        if (bulletCollider)
            bulletCollider->bIsColliding = true;
        
        const auto enqueueDamage = [&](Entity target)
            {
                if (auto eventManager = mWorld->GetEventManager())
                {
                    if (!canDamageTarget(instigator, target))
                        return;

                    EvDamage damageEvent{};
                    damageEvent.instigator = instigator;
                    damageEvent.target = target;
                    damageEvent.amount = static_cast<int32>((std::max)(0.0f, bullet->mDamage));
                    eventManager->Enqueue<EvDamage>(damageEvent);
                }
            };

        for (const BulletHitCandidate& hitCandidate : hitCandidates)
        {
            if (!shouldPenetrate && hitCandidate.Distance > nearestHitDistance)
                continue;

            if (hitCandidate.Collider)
                hitCandidate.Collider->bIsColliding = true;

            applyKnockback(hitCandidate.Target);
            enqueueDamage(hitCandidate.Target);

            if (auto eventManager = mWorld->GetEventManager())
            {
                eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
                    static_cast<uint8>(bullet->mType),
                    bulletTransform->mLocalPosition.x,
                    bulletTransform->mLocalPosition.y,
                    bulletTransform->mLocalPosition.z,
                    EffectSpawnReason::CollisionEntity,
                    bulletTransform->mLocalRotationE.x,
                    bulletTransform->mLocalRotationE.y,
                    bulletTransform->mLocalRotationE.z });

            }

            if (!shouldPenetrate)
                break;
        }

        if (!shouldPenetrate)
        {
            bullet->Deactivate();
            mWorld->UnregisterActiveBullet(bulletEntity);

            auto eventManager = mWorld->GetEventManager();
            if (eventManager)
            {
                //effectSpawn
                eventManager->Enqueue<EvBulletDeactivated>(EvBulletDeactivated{ bulletEntity });
            }
            continue;
        }

        ++i;
    }
}

void CollisionSystem::Bullet2StaticCCD(float deltaTime)
{
    (void)deltaTime;

    if (false == mWorld->HasComponentPool<BulletComponent>()) return;
    if (false == mWorld->HasComponentPool<TransformComponent>()) return;
    if (!mPhysicsWorld) return;

    auto& activeBulletEntityIds = mWorld->GetActiveBulletEntityIds();

    for (size_t i = 0; i < activeBulletEntityIds.size();)
    {
        Entity bulletEntity{ activeBulletEntityIds[i] };
        BulletComponent* bullet = mWorld->GetComponent<BulletComponent>(bulletEntity);
        TransformComponent* tr = mWorld->GetComponent<TransformComponent>(bulletEntity);
        if (!bullet || !tr || !bullet->mIsActive)
        {
            mWorld->UnregisterActiveBullet(bulletEntity);
            continue;
        }

        const Vec3 movement = tr->mMovingVector;
        if (movement.LengthSquared() <= 1e-8f)
        {
            ++i;
            continue;
        }

        Vec3 direction = movement;
        direction.Normalize();

        const Vec3 endPosition = tr->mLocalPosition;
        const Vec3 startPosition = endPosition - movement;

        float bulletRadius = 0.1f;
        BoxColliderComponent* bulletCollider = mWorld->GetComponent<BoxColliderComponent>(bulletEntity);
        if (bulletCollider)
        {
            PhysicsWorld::UpdateWorldOBB(tr, bulletCollider);
            bulletRadius = (std::max)(bulletRadius,
                (std::max)(bulletCollider->mWorldOBB.Extents.x, bulletCollider->mWorldOBB.Extents.z));
        }
        else
        {
            const float scaleRadius = (std::max)(0.1f, (std::max)(tr->mLocalScale.x, tr->mLocalScale.z) * 0.5f);
            bulletRadius = (std::max)(bulletRadius, scaleRadius);
        }

        const SweepHit hit = mPhysicsWorld->SphereSweepVsOBB(startPosition, endPosition, bulletRadius);
        if (!hit.hit)
        {
            ++i;
            continue;
        }

        tr->mLocalPosition = startPosition + direction * hit.distance;
        tr->mMovingVector = Vec3::Zero;
        bullet->Deactivate();

        if (bulletCollider)
            bulletCollider->bIsColliding = true;

        if (hit.colliderId.IsValid())
        {
            BoxColliderComponent* staticCollider = mWorld->GetComponent<BoxColliderComponent>(hit.colliderId);
            if (staticCollider)
                staticCollider->bIsColliding = true;
        }

        mWorld->UnregisterActiveBullet(bulletEntity);

        auto eventManager = mWorld->GetEventManager();
        if (eventManager)
        {
            //effectSpawn
            eventManager->Enqueue<EvBulletDeactivated>(EvBulletDeactivated{ bulletEntity });
            eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
                    static_cast<uint8>(bullet->mType),
                    tr->mLocalPosition.x,
                    tr->mLocalPosition.y,
                    tr->mLocalPosition.z,
                    EffectSpawnReason::CollisionStatic,
                    tr->mLocalRotationE.x,
                    tr->mLocalRotationE.y,
                    tr->mLocalRotationE.z });
        }
    }
}


void CollisionSystem::AvoidCollisionByMovementState(
    World* world,
    Entity a,
    Entity b,
    BoxColliderComponent* colA,
    BoxColliderComponent* colB,
    float deltaTime) // [수정]
{
    if (!world || !colA || !colB)
        return;


    if (colA->bIsTrigger || colB->bIsTrigger)
        return;

    // dt 방어
    if (deltaTime <= 0.0f)
        deltaTime = 1.0f / 60.0f; // [수정] 안전 기본값

    // A -> B (XZ 평면)
    Vec3 delta(
        colB->mWorldOBB.Center.x - colA->mWorldOBB.Center.x,
        0.0f,
        colB->mWorldOBB.Center.z - colA->mWorldOBB.Center.z);

    float lenSq = delta.x * delta.x + delta.z * delta.z;
    if (lenSq < 1e-6f)
    {
        delta = Vec3(1.0f, 0.0f, 0.0f);
        lenSq = 1.0f;
    }

    const float centerDistance = std::sqrt(lenSq);
    const float invLen = 1.0f / centerDistance;
    const Vec3 normal(delta.x * invLen, 0.0f, delta.z * invLen); // A -> B

    // --- penetration 계산(여전히 원형 근사: 빠르지만 정확도 한계 있음) ---
    // [수정] 코너 8개 순회 제거. extents 기반 XZ 외접원 반경 근사.
    auto getRadiusXZ = [](const BoundingOrientedBox& obb)
        {
            const float ex = obb.Extents.x;
            const float ez = obb.Extents.z;
            return std::sqrt(ex * ex + ez * ez);
        };

    const float radiusA = getRadiusXZ(colA->mWorldOBB);
    const float radiusB = getRadiusXZ(colB->mWorldOBB);

    // 침투량(>0이면 겹침)
    const float penetration = (radiusA + radiusB) - centerDistance;

    // ---------------------------
    // [수정] 시간 기반(바움가르테) 보정량 계산
    // push = beta/dt * (penetration - slop)
    // ---------------------------
    constexpr float kPenetrationSlop = 0.01f;   // [수정] 슬롭 감소(관통 방지에 유리)
    constexpr float kBeta = 0.20f;              // [수정] 0.1~0.4 튜닝 (클수록 빨리 밀어냄)
    constexpr float kMaxPushPerPair = 2.0f;     // [수정] 기존 0.8은 관통을 만들기 쉬움(상황 따라 1~5)

    const float effectivePenetration = (std::max)(0.0f, penetration - kPenetrationSlop);

    // [수정] 시간 기반 보정량: dt가 커지면 더 많이 보정하여 누적 침투를 억제
    float pushMagnitude = effectivePenetration * (kBeta / deltaTime);

    // [수정] 1쌍당 과도한 보정 제한
    pushMagnitude = (std::min)(pushMagnitude, kMaxPushPerPair);

    if (pushMagnitude > 0.0f)
    {
        auto* trA = world->GetComponent<TransformComponent>(a);
        auto* trB = world->GetComponent<TransformComponent>(b);

        const bool canMoveA = trA && world->HasComponent<MovableComponent>(a);
        const bool canMoveB = trB && world->HasComponent<MovableComponent>(b);

        constexpr float kMoveEpsilonSq = 1e-6f;
        const auto wasMoving = [&](TransformComponent* tr) -> bool
            {
                if (!tr) return false;
                const float vx = tr->mMovingVector.x;
                const float vz = tr->mMovingVector.z;
                return (vx * vx + vz * vz) > kMoveEpsilonSq;
            };

        const bool wasMovingA = wasMoving(trA);
        const bool wasMovingB = wasMoving(trB);

        Vec3 correctionA = Vec3::Zero;
        Vec3 correctionB = Vec3::Zero;

        // [수정] 둘 다 움직이면 반반, 하나만 움직이면 한쪽만
        if (canMoveA && canMoveB && wasMovingA && wasMovingB)
        {
            const float half = pushMagnitude * 0.5f;
            correctionA = Vec3(-normal.x * half, 0.0f, -normal.z * half);
            correctionB = Vec3(normal.x * half, 0.0f, normal.z * half);
        }
        else if (canMoveA && wasMovingA)
        {
            correctionA = Vec3(-normal.x * pushMagnitude, 0.0f, -normal.z * pushMagnitude);
        }
        else if (canMoveB && wasMovingB)
        {
            correctionB = Vec3(normal.x * pushMagnitude, 0.0f, normal.z * pushMagnitude);
        }

        // [수정] Transform 변경 + OBB center 동기화
        // 주의: mLocalPosition만 바꾸면 mWorldMatrix가 즉시 갱신되지 않을 수 있음.
        // TransformSystem이 dirty를 기반으로 WorldMatrix를 재계산한다면 여기서 dirty를 세워야 한다.
        if (canMoveA)
        {
            trA->mLocalPosition += correctionA;

            // trA->MarkDirty(); // [수정] 네 엔진에 맞는 dirty 훅이 있으면 꼭 호출

            colA->mWorldOBB.Center.x += correctionA.x;
            colA->mWorldOBB.Center.z += correctionA.z;
        }

        if (canMoveB)
        {
            trB->mLocalPosition += correctionB;

            // trB->MarkDirty(); // [수정] 네 엔진에 맞는 dirty 훅이 있으면 꼭 호출

            colB->mWorldOBB.Center.x += correctionB.x;
            colB->mWorldOBB.Center.z += correctionB.z;
        }
    }

    // ---------------------------
    // 이동 중이던 엔티티만 정지 처리
    // ---------------------------
    auto stopIfMoving = [&](Entity e)
        {
            auto* tr = world->GetComponent<TransformComponent>(e);
            if (!tr)
                return;

            const float v2 = tr->mMovingVector.x * tr->mMovingVector.x + tr->mMovingVector.z * tr->mMovingVector.z;
            if (v2 <= 1e-6f)
                return;

            tr->mMovingVector.x = 0.0f;
            tr->mMovingVector.z = 0.0f;

            if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(e))
                enemyMove->mMovingDirection = Vec3::Zero;

            if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(e))
                playerMove->mMovingDirection = Vec3::Zero;

            if (auto* inputComp = world->GetComponent<InputComponent>(e))
            {
                inputComp->MoveX = 0.0f;
                inputComp->MoveZ = 0.0f;
            }
        };

    stopIfMoving(a);
    stopIfMoving(b);
}

