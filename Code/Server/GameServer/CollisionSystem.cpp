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
    constexpr float kGuitarAttack2ExplosionRadius = 300.0f;
    constexpr float kGuitarAttack2ExplosionDamageScale = 0.6f;

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
    std::vector<SpatialGridItem2D> gridItems;
    std::unordered_map<EntityID, BoxColliderComponent*> colliderByEntityId;
    std::unordered_map<EntityID, SpatialGridBounds2D> boundsByEntityId;

    activeEntities.reserve(entities.size());
    colliders.reserve(entities.size());
    gridItems.reserve(entities.size());
    colliderByEntityId.reserve(entities.size());
    boundsByEntityId.reserve(entities.size());

    // (B) 월드 OBB 갱신 + SoA 데이터 채우기
    for (auto e : entities)
    {
        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (col) col->bIsColliding = false;
        if (!tr || !col) continue;
        if (IsDeadEnemy(mWorld, e)) continue;

        PhysicsWorld::UpdateWorldOBB(tr, col);
        const AABB2D aabb = PhysicsWorld::BuildAABBFromOBB(col->mWorldOBB);

        activeEntities.push_back(e);
        colliders.push_back(col);
        colliderByEntityId[e.GetID()] = col;
        boundsByEntityId[e.GetID()] = { aabb.minX, aabb.maxX, aabb.minZ, aabb.maxZ };

        SpatialGridItem2D item;
        item.entity = e;
        item.position = tr->mLocalPosition;
        item.bounds.minX = aabb.minX;
        item.bounds.maxX = aabb.maxX;
        item.bounds.minZ = aabb.minZ;
        item.bounds.maxZ = aabb.maxZ;
        gridItems.push_back(item);
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
                        const SpatialGridBounds2D& lhsBounds = boundsByEntityId[activeEntities[lhs].GetID()];
                        const SpatialGridBounds2D& rhsBounds = boundsByEntityId[activeEntities[rhs].GetID()];
                        if (lhsBounds.minX == rhsBounds.minX)
                            return lhsBounds.maxX < rhsBounds.maxX;
                        return lhsBounds.minX < rhsBounds.minX;
                    });

                std::vector<uint32_t> activeList;
                activeList.reserve(order.size());

                for (uint32_t currentIndex : order)
                {
                    const SpatialGridBounds2D& currentBounds = boundsByEntityId[activeEntities[currentIndex].GetID()];
                    const float currentMinX = currentBounds.minX;

                    activeList.erase(
                        std::remove_if(activeList.begin(), activeList.end(), [&](uint32_t otherIndex)
                            {
                                const SpatialGridBounds2D& otherBounds = boundsByEntityId[activeEntities[otherIndex].GetID()];
                                return otherBounds.maxX < currentMinX;
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
                        const SpatialGridBounds2D& otherBounds = boundsByEntityId[activeEntities[otherIndex].GetID()];
                        if (currentBounds.maxZ < otherBounds.minZ || otherBounds.maxZ < currentBounds.minZ)
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

    std::vector<std::pair<Entity, Entity>> candidatePairs;
    if (mPhysicsWorld)
        mPhysicsWorld->GetMovableCollisionPairs(candidatePairs);

    if (!mPhysicsWorld || candidatePairs.empty())
    {
        SpatialGrid2D localGrid;
        localGrid.Build(gridItems);

        if (localGrid.Empty())
        {
            runSAP();
            return;
        }

        localGrid.GetCandidatePairs(candidatePairs);
    }

    for (const auto& [entityA, entityB] : candidatePairs)
    {
        auto findColA = colliderByEntityId.find(entityA.GetID());
        auto findColB = colliderByEntityId.find(entityB.GetID());
        if (findColA == colliderByEntityId.end() || findColB == colliderByEntityId.end())
            continue;

        BoxColliderComponent* colA = findColA->second;
        BoxColliderComponent* colB = findColB->second;
        if (!colA || !colB)
            continue;

        if (colA->mWorldOBB.Intersects(colB->mWorldOBB))
        {
            colA->bIsColliding = true;
            colB->bIsColliding = true;

            AvoidCollisionByMovementState(
                mWorld,
                entityA,
                entityB,
                colA,
                colB,
                deltaTime);
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
        
        BoxColliderComponent* staticCollider = mWorld->GetComponent<BoxColliderComponent>(st.ColliderEntity);
        st.ColliderBox = staticCollider;
        if (staticCollider)
            staticCollider->bIsColliding = false;

        SphereColliderComponent* staticSphere = mWorld->HasComponentPool<SphereColliderComponent>()
            ? mWorld->GetComponent<SphereColliderComponent>(st.ColliderEntity)
            : nullptr;
        if (staticSphere)
            staticSphere->bIsColliding = false;
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
            if (st.shape == StaticProxyShape::Sphere)
            {
                SphereColliderComponent* staticSphere = mWorld->GetComponent<SphereColliderComponent>(st.ColliderEntity);
                TransformComponent* staticTransform = mWorld->GetComponent<TransformComponent>(st.ColliderEntity);
                if (!staticSphere || !staticTransform)
                    continue;

                // 수정 내용
                // CRX_Sphere 는 JSON 에서 비균일 scale 을 가질 수 있으므로 클라이언트 렌더링과 같은 TRS 의 bounds 로 검사한다.
                // broad phase 후보는 BVH 에서 받은 뒤 narrow phase 에서 동적 OBB 와 CRX_Sphere bounds 를 검사한다.
                PhysicsWorld::UpdateWorldSphere(staticTransform, staticSphere);
                st.sphere = staticSphere->mWorldSphere;
                st.sphereBounds = staticSphere->mWorldBounds;
                NormalizeCollisionOBB(dyn.collider);

                if (!dyn.collider->mWorldOBB.Intersects(staticSphere->mWorldBounds))
                    continue;

                dyn.collider->bIsColliding = true;
                staticSphere->bIsColliding = true;

                AvoidCollisionWithStaticSphere(
                    mWorld,
                    dyn.entity,
                    dyn.collider,
                    staticSphere->mWorldBounds,
                    deltaTime);
                continue;
            }
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

        const bool shouldPenetrate = bullet->mPenetrates;
        if (!shouldPenetrate)
        {
            bulletTransform->mLocalPosition = startPosition + direction * nearestHitDistance;
            bulletTransform->mMovingVector = Vec3::Zero;
        }

        auto applyKnockback = [&](Entity target)    // Knockback
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
                    if (auto* player = mWorld->GetComponent<MainPlayerComponent>(target))
                    {
                       
                        player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::OverrideXZ);
                        player->mExternalVelocity = knockbackVector;
                        player->mExternalMoveEndTime = GetServerTotalTimeSeconds() + 0.25f;
                    }
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
                    damageEvent.skillType = bullet->mType;
                    damageEvent.isCritical = bullet->mIsCritical;
                    eventManager->Enqueue<EvDamage>(damageEvent);
                }
            };

        const auto enqueueExplosionDamage = [&](const Vec3& impactPosition, Entity directTarget)
            {
                if (bullet->mType != SkillType::GuitarAttack_2)
                    return;
                if (!mWorld->HasComponent<EnemyComponent>(directTarget))
                    return;

                auto eventManager = mWorld->GetEventManager();
                if (!eventManager)
                    return;

                const float radiusSq = kGuitarAttack2ExplosionRadius * kGuitarAttack2ExplosionRadius;
                const int32 explosionDamage = static_cast<int32>(
                    (std::max)(0.0f, bullet->mDamage * kGuitarAttack2ExplosionDamageScale));
                if (explosionDamage <= 0)
                    return;

                for (Entity splashTarget : dynamicEntities)
                {
                    if (!splashTarget.IsValid() || splashTarget == directTarget || splashTarget == bulletEntity)
                        continue;
                    if (IsDeadEnemy(mWorld, splashTarget))
                        continue;
                    if (!canDamageTarget(instigator, splashTarget))
                        continue;

                    TransformComponent* splashTransform = mWorld->GetComponent<TransformComponent>(splashTarget);
                    if (!splashTransform)
                        continue;

                    Vec3 delta = splashTransform->mLocalPosition - impactPosition;
                    if (delta.LengthSquared() > radiusSq)
                        continue;

                    EvDamage splashDamage{};
                    splashDamage.instigator = instigator;
                    splashDamage.target = splashTarget;
                    splashDamage.amount = explosionDamage;
                    splashDamage.skillType = bullet->mType;
                    splashDamage.isCritical = bullet->mIsCritical;
                    eventManager->Enqueue<EvDamage>(splashDamage);
                }
            };

        for (const BulletHitCandidate& hitCandidate : hitCandidates)
        {
            if (!shouldPenetrate && hitCandidate.Distance > nearestHitDistance)
                continue;

            if (shouldPenetrate && !bullet->TryRegisterHitTarget(hitCandidate.Target))
                continue;

            const Vec3 impactPosition = startPosition + direction * hitCandidate.Distance;

            if (hitCandidate.Collider)
                hitCandidate.Collider->bIsColliding = true;

            applyKnockback(hitCandidate.Target);
            enqueueDamage(hitCandidate.Target);
            enqueueExplosionDamage(impactPosition, hitCandidate.Target);

            if (auto eventManager = mWorld->GetEventManager())
            {
                eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
                    static_cast<uint8>(bullet->mType),
                   
                    impactPosition.x,
                    impactPosition.y,
                    impactPosition.z,
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

        if (bullet->mPenetratesStatic)
        {
            ++i;
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


        JoltStaticHit joltHit{};
        const bool hitJoltStatic = mPhysicsWorld->CastMovingSphereAgainstStatic(
            startPosition,
            endPosition,
            bulletRadius,
            joltHit);

        SweepHit legacyHit{};
        if (!hitJoltStatic)
            legacyHit = mPhysicsWorld->SphereSweepVsOBB(startPosition, endPosition, bulletRadius);

        if (!hitJoltStatic && !legacyHit.hit)
        {
            ++i;
            continue;
        }

        const Vec3 impactPosition = hitJoltStatic
            ? joltHit.point
            : startPosition + direction * legacyHit.distance;

        tr->mLocalPosition = impactPosition;
        tr->mMovingVector = Vec3::Zero;
        bullet->Deactivate();

        if (bulletCollider)
            bulletCollider->bIsColliding = true;

        const Entity staticColliderEntity = hitJoltStatic ? joltHit.colliderId : legacyHit.colliderId;
        if (staticColliderEntity.IsValid())
        {
            BoxColliderComponent* staticCollider = mWorld->GetComponent<BoxColliderComponent>(staticColliderEntity);
            if (staticCollider)
                staticCollider->bIsColliding = true;

            SphereColliderComponent* staticSphere = mWorld->HasComponentPool<SphereColliderComponent>()
                ? mWorld->GetComponent<SphereColliderComponent>(staticColliderEntity)
                : nullptr;
            if (staticSphere)
                staticSphere->bIsColliding = true;
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

void CollisionSystem::AvoidCollisionWithStaticSphere(
    World* world,
    Entity dynamicEntity,
    BoxColliderComponent* dynamicCollider,
    const BoundingOrientedBox& staticSphereBounds,
    float deltaTime)
{
    if (!world || !dynamicCollider)
        return;

    if (deltaTime <= 0.0f)
        deltaTime = 1.0f / 60.0f;

    TransformComponent* dynamicTransform = world->GetComponent<TransformComponent>(dynamicEntity);
    if (!dynamicTransform || !world->HasComponent<MovableComponent>(dynamicEntity))
        return;

    const Vec3 dynamicCenter = dynamicCollider->mWorldOBB.Center;
    const Vec3 sphereCenter = staticSphereBounds.Center;
    Vec3 delta(
        dynamicCenter.x - sphereCenter.x,
        0.0f,
        dynamicCenter.z - sphereCenter.z);

    float lenSq = delta.x * delta.x + delta.z * delta.z;
    if (lenSq < 1e-6f)
    {
        delta = dynamicTransform->mMovingVector;
        delta.y = 0.0f;
        lenSq = delta.x * delta.x + delta.z * delta.z;
        if (lenSq < 1e-6f)
        {
            delta = Vec3(1.0f, 0.0f, 0.0f);
            lenSq = 1.0f;
        }
    }

    const float centerDistance = std::sqrt(lenSq);
    const float invLen = 1.0f / centerDistance;
    const Vec3 normal(delta.x * invLen, 0.0f, delta.z * invLen);

    const float ex = dynamicCollider->mWorldOBB.Extents.x;
    const float ez = dynamicCollider->mWorldOBB.Extents.z;
    const float dynamicRadius = std::sqrt(ex * ex + ez * ez);
    const float sx = staticSphereBounds.Extents.x;
    const float sz = staticSphereBounds.Extents.z;
    const float staticRadius = std::sqrt(sx * sx + sz * sz);
    const float penetration = (dynamicRadius + staticRadius) - centerDistance;

    constexpr float kPenetrationSlop = 0.01f;
    constexpr float kBeta = 0.20f;
    constexpr float kMaxPushPerPair = 2.0f;

    const float effectivePenetration = (std::max)(0.0f, penetration - kPenetrationSlop);
    float pushMagnitude = effectivePenetration * (kBeta / deltaTime);
    pushMagnitude = (std::min)(pushMagnitude, kMaxPushPerPair);

    const float v2 =
        dynamicTransform->mMovingVector.x * dynamicTransform->mMovingVector.x +
        dynamicTransform->mMovingVector.z * dynamicTransform->mMovingVector.z;

   
    if (pushMagnitude > 0.0f && v2 > 1e-6f)
    {
        const Vec3 correction(normal.x * pushMagnitude, 0.0f, normal.z * pushMagnitude);
        dynamicTransform->mLocalPosition += correction;
        dynamicCollider->mWorldOBB.Center.x += correction.x;
        dynamicCollider->mWorldOBB.Center.z += correction.z;
    }

    dynamicTransform->mMovingVector.x = 0.0f;
    dynamicTransform->mMovingVector.z = 0.0f;

    if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(dynamicEntity))
        enemyMove->mMovingDirection = Vec3::Zero;

    if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(dynamicEntity))
        playerMove->mMovingDirection = Vec3::Zero;

    if (auto* inputComp = world->GetComponent<InputComponent>(dynamicEntity))
    {
        inputComp->MoveX = 0.0f;
        inputComp->MoveZ = 0.0f;
    }
}
