#include "pch.h"
#include "World.h"
#include "ResourceManager.h"
#include "CollisionSystem.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "TagComponent.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);
static void AvoidCollisionByMovementState(
    World* world,
    Entity a,
    Entity b,
    BoxColliderComponent* colA,
    BoxColliderComponent* colB);

namespace
{
    struct AABB2D
    {
        float minX;
        float maxX;
        float minZ;
        float maxZ;
    };

    struct StaticProxy
    {
        Entity entity;
        BoxColliderComponent* collider;
        AABB2D bounds;
    };

    struct DynamicProxy
    {
        Entity entity;
        BoxColliderComponent* collider;
        AABB2D bounds;
    };

    struct BVHNode
    {
        AABB2D bounds;
        int left = -1;
        int right = -1;
        int start = 0;
        int count = 0;

        bool IsLeaf() const
        {
            return left < 0 && right < 0;
        }
    };

    AABB2D BuildAABBFromOBB(const BoundingOrientedBox& obb)
    {
        XMFLOAT3 corners[8];
        obb.GetCorners(corners);

        AABB2D bounds{ corners[0].x, corners[0].x, corners[0].z, corners[0].z };

        for (const auto& c : corners)
        {
            bounds.minX = (std::min)(bounds.minX, c.x);
            bounds.maxX = (std::max)(bounds.maxX, c.x);
            bounds.minZ = (std::min)(bounds.minZ, c.z);
            bounds.maxZ = (std::max)(bounds.maxZ, c.z);
        }

        return bounds;
    }

    AABB2D MergeAABB(const AABB2D& a, const AABB2D& b)
    {
        return AABB2D{
            (std::min)(a.minX, b.minX),
            (std::max)(a.maxX, b.maxX),
            (std::min)(a.minZ, b.minZ),
            (std::max)(a.maxZ, b.maxZ)
        };
    }

    bool OverlapAABB(const AABB2D& a, const AABB2D& b)
    {
        if (a.maxX < b.minX || b.maxX < a.minX)
            return false;
        if (a.maxZ < b.minZ || b.maxZ < a.minZ)
            return false;
        return true;
    }

    int BuildStaticBVHRecursive(
        std::vector<StaticProxy>& proxies,
        std::vector<BVHNode>& nodes,
        int start,
        int count)
    {
        const int nodeIndex = static_cast<int>(nodes.size());
        nodes.push_back(BVHNode{});

        BVHNode& node = nodes[nodeIndex];
        node.start = start;
        node.count = count;
        node.bounds = proxies[start].bounds;

        for (int i = 1; i < count; ++i)
        {
            node.bounds = MergeAABB(node.bounds, proxies[start + i].bounds);
        }

        constexpr int kLeafSize = 4;
        if (count <= kLeafSize)
            return nodeIndex;

        const float extentX = node.bounds.maxX - node.bounds.minX;
        const float extentZ = node.bounds.maxZ - node.bounds.minZ;
        const bool splitX = extentX >= extentZ;

        const int mid = start + count / 2;
        std::nth_element(
            proxies.begin() + start,
            proxies.begin() + mid,
            proxies.begin() + start + count,
            [splitX](const StaticProxy& lhs, const StaticProxy& rhs)
            {
                const float lhsCenter = splitX
                    ? (lhs.bounds.minX + lhs.bounds.maxX) * 0.5f
                    : (lhs.bounds.minZ + lhs.bounds.maxZ) * 0.5f;
                const float rhsCenter = splitX
                    ? (rhs.bounds.minX + rhs.bounds.maxX) * 0.5f
                    : (rhs.bounds.minZ + rhs.bounds.maxZ) * 0.5f;
                return lhsCenter < rhsCenter;
            });

        node.left = BuildStaticBVHRecursive(proxies, nodes, start, mid - start);
        node.right = BuildStaticBVHRecursive(proxies, nodes, mid, start + count - mid);
        node.count = 0;
        return nodeIndex;
    }

    void QueryStaticBVH(
        const std::vector<BVHNode>& nodes,
        int root,
        const AABB2D& query,
        std::vector<int>& outIndices)
    {
        if (root < 0) return;

        std::vector<int> stack;                 // [수정] 동적 스택으로 안전하게
        stack.reserve(64);
        stack.push_back(root);

        while (!stack.empty())
        {
            const int nodeIndex = stack.back();
            stack.pop_back();

            const BVHNode& node = nodes[nodeIndex];

            if (!OverlapAABB(node.bounds, query))
                continue;

            if (node.IsLeaf())
            {
                for (int i = 0; i < node.count; ++i)
                    outIndices.push_back(node.start + i);
                continue;
            }

            if (node.left >= 0)  stack.push_back(node.left);
            if (node.right >= 0) stack.push_back(node.right);
        }
    }
}

CollisionSystem::CollisionSystem(World* world) : System(world)
{

}

void CollisionSystem::Update(float dt)
{
    if (false == mWorld->HasComponentPool<BoxColliderComponent>())return;
    if (false == mWorld->HasComponentPool<TransformComponent>())return;


    Movable2Movable(dt);
    Movable2Static(dt);

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

        UpdateWorldOBB(tr, col);

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
                            colB);
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

    
    std::unordered_set<uint64_t> checkedPairs;
    checkedPairs.reserve(indices.size());

    
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
                            colB);
                    }
                }
            }
        }
}

void CollisionSystem::Movable2Static(float deltaTime)
{
    if (false == mWorld->HasComponentPool<MovableComponent>())return;
    if (false == mWorld->HasComponentPool<StaticComponent>())return;

    auto dynamicEntities = mWorld->GetEntitiesWithComponents<MovableComponent, TransformComponent, BoxColliderComponent>();
    auto staticEntities = mWorld->GetEntitiesWithComponents<StaticComponent, TransformComponent, BoxColliderComponent>();

    std::vector<StaticProxy> staticObjects;
    std::vector<DynamicProxy> dynamicObjects;

    staticObjects.reserve(staticEntities.size());
    dynamicObjects.reserve(dynamicEntities.size());

    for (auto e : staticEntities)
    {
        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (!tr || !col)
            continue;

        UpdateWorldOBB(tr, col);
        const AABB2D bounds = BuildAABBFromOBB(col->mWorldOBB);

        staticObjects.push_back(StaticProxy{ e, col, bounds });
    }
    for (auto e : dynamicEntities)
    {
        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (!tr || !col)
            continue;

        UpdateWorldOBB(tr, col);
        const AABB2D bounds = BuildAABBFromOBB(col->mWorldOBB);

        dynamicObjects.push_back(DynamicProxy{ e, col, bounds });
    }

    if (staticObjects.empty() || dynamicObjects.empty())
        return;

    std::vector<BVHNode> nodes;
    nodes.reserve(staticObjects.size() * 2);
    const int root = BuildStaticBVHRecursive(staticObjects, nodes, 0, static_cast<int>(staticObjects.size()));

    std::vector<int> candidates;
    for (auto& dyn : dynamicObjects)
    {
        candidates.clear();
        QueryStaticBVH(nodes, root, dyn.bounds, candidates);

        for (int candidateIndex : candidates)
        {
            auto& st = staticObjects[candidateIndex];

            if (!dyn.collider->mWorldOBB.Intersects(st.collider->mWorldOBB))
                continue;

            dyn.collider->bIsColliding = true;
            st.collider->bIsColliding = true;

            AvoidCollisionByMovementState(
                mWorld,
                dyn.entity,
                st.entity,
                dyn.collider,
                st.collider);
        }
    }
}






//////

static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col)
{
    XMVECTOR S, R, T;

    // [수정] SimpleMath::Matrix -> XMMATRIX 변환
    const XMMATRIX M = tr->mWorldMatrix; // SimpleMath::Matrix는 XMMATRIX로 암시 변환되는 경우가 많음

    if (!XMMatrixDecompose(&S, &R, &T, M))
        return;

    // scale / rotation(quat) / translation 추출
    const XMFLOAT3 s3 = {};
    const XMFLOAT4 r4 = {};
    const XMFLOAT3 t3 = {};
    XMFLOAT3 sF, tF;
    XMFLOAT4 rF;
    XMStoreFloat3(&sF, S);
    XMStoreFloat3(&tF, T);
    XMStoreFloat4(&rF, XMQuaternionNormalize(R));

    const Vec3 worldPos = Vec3(tF.x, tF.y, tF.z);

    // 로컬 Center 오프셋을 월드 회전으로 회전
    const XMVECTOR localCenter = XMVectorSet(col->mCenter.x, col->mCenter.y, col->mCenter.z, 0.0f);
    const XMVECTOR rotatedOffV = XMVector3Rotate(localCenter, XMLoadFloat4(&rF));
    XMFLOAT3 rotatedOffF;
    XMStoreFloat3(&rotatedOffF, rotatedOffV);

    const Vec3 worldCenter = worldPos + Vec3(rotatedOffF.x, rotatedOffF.y, rotatedOffF.z);

    // Extents
    Vec3 ext = col->mHalfExtents;


    col->mWorldOBB.Center = XMFLOAT3(worldCenter.x, worldCenter.y, worldCenter.z);
    col->mWorldOBB.Extents = XMFLOAT3(ext.x, ext.y, ext.z);
    col->mWorldOBB.Orientation = XMFLOAT4(rF.x, rF.y, rF.z, rF.w);
}
/*
static void AvoidCollisionByMovementState(
    World* world,
    Entity a,
    Entity b,
    BoxColliderComponent* colA,
    BoxColliderComponent* colB)
{
    if (!world || !colA || !colB)
        return;

    Vec3 delta(
        colB->mWorldOBB.Center.x - colA->mWorldOBB.Center.x,
        0.0f,
        colB->mWorldOBB.Center.z - colA->mWorldOBB.Center.z);

    float lenSq = delta.x * delta.x + delta.z * delta.z;
    if (lenSq < 1e-6f)
    {
        // 중심이 거의 동일하면 고정 방향으로 회피 축 지정
        delta = Vec3(1.0f, 0.0f, 0.0f);
        lenSq = 1.0f;
    }

    const float invLen = 1.0f / std::sqrt(lenSq);
    const Vec3 normal(delta.x * invLen, 0.0f, delta.z * invLen);          // A -> B
    const Vec3 tangent(-normal.z, 0.0f, normal.x);                          // 평면 접선

    auto getRadiusXZ = [](const BoundingOrientedBox& obb)
        {
            XMFLOAT3 corners[8];
            obb.GetCorners(corners);

            float radius = 0.0f;
            for (const auto& c : corners)
            {
                const float dx = c.x - obb.Center.x;
                const float dz = c.z - obb.Center.z;
                radius = (std::max)(radius, std::sqrt(dx * dx + dz * dz));
            }

            return radius;
        };

    const float radiusA = getRadiusXZ(colA->mWorldOBB);
    const float radiusB = getRadiusXZ(colB->mWorldOBB);
    const float centerDistance = std::sqrt(lenSq);
    const float penetration = (radiusA + radiusB) - centerDistance;

    // 과도한 튕김 방지를 위한 완화 파라미터
    constexpr float kPenetrationSlop = 0.05f;   // 이 값 이하는 무시
    constexpr float kPushStrength = 0.35f;      // 침투량 대비 보정 비율
    constexpr float kMaxPushPerPair = 0.8f;     // 1회 충돌당 최대 보정량

    const float effectivePenetration = (std::max)(0.0f, penetration - kPenetrationSlop);
    const float pushMagnitude = (std::min)(kMaxPushPerPair, effectivePenetration * kPushStrength);


    if (penetration > 0)
    {
        auto* trA = world->GetComponent<TransformComponent>(a);
        auto* trB = world->GetComponent<TransformComponent>(b);

        const bool canMoveA = trA && world->HasComponent<MovableComponent>(a);
        const bool canMoveB = trB && world->HasComponent<MovableComponent>(b);

        Vec3 correctionA = Vec3::Zero;
        Vec3 correctionB = Vec3::Zero;

        if (canMoveA && canMoveB)
        {
            const float half = penetration * 0.5f;
            correctionA = Vec3(-normal.x * half, 0.0f, -normal.z * half);
            correctionB = Vec3(normal.x * half, 0.0f, normal.z * half);
        }
        else if (canMoveA)
        {
            correctionA = Vec3(-normal.x * pushMagnitude, 0.0f, -normal.z * pushMagnitude);
        }
        else if (canMoveB)
        {
            correctionB = Vec3(normal.x * pushMagnitude, 0.0f, normal.z * pushMagnitude);
        }

        if (canMoveA)
        {
            trA->mLocalPosition += correctionA;
            colA->mWorldOBB.Center.x += correctionA.x;
            colA->mWorldOBB.Center.z += correctionA.z;
        }

        if (canMoveB)
        {
            trB->mLocalPosition += correctionB;
            colB->mWorldOBB.Center.x += correctionB.x;
            colB->mWorldOBB.Center.z += correctionB.z;
        }
    }



    auto steerMovementState = [&](Entity e, const Vec3& towardOther, float tangentSign)
        {
            Vec3 dir(0.0f, 0.0f, 0.0f);
            bool hasDir = false;

            if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(e))
            {
                dir = enemyMove->mMovingDirection;
                hasDir = true;
            }
            else if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(e))
            {
                dir = playerMove->mMovingDirection;
                hasDir = true;
            }

            if (!hasDir)
                return;

            // 상대쪽으로 파고드는 성분 제거
            const float towardDot = dir.x * towardOther.x + dir.z * towardOther.z;
            if (towardDot > 0.0f)
            {
                dir.x -= towardOther.x * towardDot;
                dir.z -= towardOther.z * towardDot;
            }

            // 좌/우로 비켜가도록 접선 성분 추가
            dir += tangent * (0.25f * tangentSign);

            const float d2 = dir.x * dir.x + dir.z * dir.z;
            if (d2 < 1e-6f)
            {
                // 이동 방향이 거의 0이면 일단 뒤로 물러나며 회피
                dir = Vec3(-towardOther.x, -towardOther.y, -towardOther.z) + tangent * (0.2f * tangentSign);
            }

            const float d2n = dir.x * dir.x + dir.z * dir.z;
            if (d2n > 1e-6f)
            {
                const float inv = 1.0f / std::sqrt(d2n);
                dir.x *= inv;
                dir.z *= inv;
            }

            if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(e))
            {
                enemyMove->mMovingDirection = dir;
            }
            if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(e))
            {
                playerMove->mMovingDirection = dir;
            }
            if (auto* inputComp = world->GetComponent<InputComponent>(e))
            {
                inputComp->MoveX = dir.x;
                inputComp->MoveZ = dir.z;
            }
        };

    // id 기반으로 좌/우 방향을 고정해 프레임간 진동 감소
    const float signA = (a.GetID() < b.GetID()) ? 1.0f : -1.0f;
    const float signB = -signA;

    steerMovementState(a, normal, signA);

    const Vec3 towardA(-normal.x, -normal.y, -normal.z);
    steerMovementState(b, towardA, signB);
}*/

static void AvoidCollisionByMovementState(
    World* world,
    Entity a,
    Entity b,
    BoxColliderComponent* colA,
    BoxColliderComponent* colB)
{
    if (!world || !colA || !colB)
        return;

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

    const float invLen = 1.0f / std::sqrt(lenSq);
    const Vec3 normal(delta.x * invLen, 0.0f, delta.z * invLen);
    const Vec3 tangent(-normal.z, 0.0f, normal.x);

    // [수정] 코너 8개+sqrt 반복 제거:
    // OBB의 XZ 외접원 반경 근사 = sqrt(ext.x^2 + ext.z^2)
    auto getRadiusXZ = [](const BoundingOrientedBox& obb)
        {
            const float ex = obb.Extents.x;
            const float ez = obb.Extents.z;
            return std::sqrt(ex * ex + ez * ez);
        };

    const float radiusA = getRadiusXZ(colA->mWorldOBB);
    const float radiusB = getRadiusXZ(colB->mWorldOBB);
    const float centerDistance = std::sqrt(lenSq);
    const float penetration = (radiusA + radiusB) - centerDistance;

    // 완화 파라미터
    constexpr float kPenetrationSlop = 0.05f;
    constexpr float kPushStrength = 0.35f;
    constexpr float kMaxPushPerPair = 0.8f;

    // [수정] 계산한 완화 파라미터를 실제 보정에 사용
    const float effectivePenetration = (std::max)(0.0f, penetration - kPenetrationSlop);
    const float pushMagnitude = (std::min)(kMaxPushPerPair, effectivePenetration * kPushStrength);

    if (pushMagnitude > 0.0f) // [수정] penetration이 아니라 pushMagnitude 기준
    {
        auto* trA = world->GetComponent<TransformComponent>(a);
        auto* trB = world->GetComponent<TransformComponent>(b);

        const bool canMoveA = trA && world->HasComponent<MovableComponent>(a);
        const bool canMoveB = trB && world->HasComponent<MovableComponent>(b);

        Vec3 correctionA = Vec3::Zero;
        Vec3 correctionB = Vec3::Zero;

        // [수정] penetration 대신 pushMagnitude 적용 (과보정/진동 완화)
        if (canMoveA && canMoveB)
        {
            const float half = pushMagnitude * 0.5f;
            correctionA = Vec3(-normal.x * half, 0.0f, -normal.z * half);
            correctionB = Vec3(normal.x * half, 0.0f, normal.z * half);
        }
        else if (canMoveA)
        {
            correctionA = Vec3(-normal.x * pushMagnitude, 0.0f, -normal.z * pushMagnitude);
        }
        else if (canMoveB)
        {
            correctionB = Vec3(normal.x * pushMagnitude, 0.0f, normal.z * pushMagnitude);
        }

        if (canMoveA)
        {
            trA->mLocalPosition += correctionA;

            // [수정] 트랜스폼 변경 시 월드행렬 재계산이 필요(엔진 구조에 맞게)
            // 아래 MarkDirty()는 예시. 네 TransformSystem이 mLocalPosition 변경을 감지 못하면 꼭 필요.
            // trA->MarkDirty();  // [수정] (함수 없으면 너 엔진 방식으로 교체)

            colA->mWorldOBB.Center.x += correctionA.x;
            colA->mWorldOBB.Center.z += correctionA.z;
        }

        if (canMoveB)
        {
            trB->mLocalPosition += correctionB;

            // trB->MarkDirty();  // [수정] (함수 없으면 너 엔진 방식으로 교체)

            colB->mWorldOBB.Center.x += correctionB.x;
            colB->mWorldOBB.Center.z += correctionB.z;
        }
    }

    auto steerMovementState = [&](Entity e, const Vec3& towardOther, float tangentSign)
        {
            Vec3 dir(0.0f, 0.0f, 0.0f);
            bool hasDir = false;

            if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(e))
            {
                dir = enemyMove->mMovingDirection;
                hasDir = true;
            }
            else if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(e))
            {
                dir = playerMove->mMovingDirection;
                hasDir = true;
            }

            if (!hasDir) return;

            const float towardDot = dir.x * towardOther.x + dir.z * towardOther.z;
            if (towardDot > 0.0f)
            {
                dir.x -= towardOther.x * towardDot;
                dir.z -= towardOther.z * towardDot;
            }

            dir += tangent * (0.25f * tangentSign);

            float d2 = dir.x * dir.x + dir.z * dir.z;
            if (d2 < 1e-6f)
            {
                dir = Vec3(-towardOther.x, 0.0f, -towardOther.z) + tangent * (0.2f * tangentSign);
                d2 = dir.x * dir.x + dir.z * dir.z;
            }

            if (d2 > 1e-6f)
            {
                const float inv = 1.0f / std::sqrt(d2);
                dir.x *= inv;
                dir.z *= inv;
            }

            if (auto* enemyMove = world->GetComponent<EnemyMovementComponent>(e))
                enemyMove->mMovingDirection = dir;

            if (auto* playerMove = world->GetComponent<PlayerMovementComponent>(e))
                playerMove->mMovingDirection = dir;

            if (auto* inputComp = world->GetComponent<InputComponent>(e))
            {
                inputComp->MoveX = dir.x;
                inputComp->MoveZ = dir.z;
            }
        };

    const float signA = (a.GetID() < b.GetID()) ? 1.0f : -1.0f;
    const float signB = -signA;

    steerMovementState(a, normal, signA);
    const Vec3 towardA(-normal.x, 0.0f, -normal.z);
    steerMovementState(b, towardA, signB);
}