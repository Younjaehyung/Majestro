#include "pch.h"
#include "World.h"
#include "ResourceManager.h"
#include "CollisionSystem.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "MovementComponent.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);

CollisionSystem::CollisionSystem(World* world) : System(world)
{

}

void CollisionSystem::Update(float dt)
{
    if (false == mWorld->HasComponentPool<BoxColliderComponent>())return;
    if (false == mWorld->HasComponentPool<TransformComponent>())return;


    Movable2Movable(dt);

    //auto entities = mWorld->GetEntitiesWithComponents<TransformComponent, BoxColliderComponent>();

    //// (B) 월드 OBB 갱신
    //for (auto e : entities)
    //{
    //    auto* tr = mWorld->GetComponent<TransformComponent>(e);
    //    auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
    //    if (col) col->bIsColliding = false;
    //    if (!tr || !col) continue;

    //    UpdateWorldOBB(tr, col);
    //}

    //// (C) 충돌 판정 (N^2)
    //for (size_t i = 0; i < entities.size(); ++i)
    //{
    //    auto a = entities[i];
    //    auto* colA = mWorld->GetComponent<BoxColliderComponent>(a);

    //    for (size_t j = i + 1; j < entities.size(); ++j)
    //    {
    //        auto b = entities[j];
    //        auto* colB = mWorld->GetComponent<BoxColliderComponent>(b);

    //        if (!colA || !colB) continue;

    //        if (colA->mWorldOBB.Intersects(colB->mWorldOBB))
    //        {
    //            colA->bIsColliding = true; // [추가]
    //            colB->bIsColliding = true; // [추가]
    //        }
    //    }
    //}

}

void CollisionSystem::Movable2Movable(float deltaTime)
{
    auto entities = mWorld->GetEntitiesWithComponents<TransformComponent, BoxColliderComponent>();

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

    auto runN2 = [&]()
        {
            for (size_t i = 0; i < activeEntities.size(); ++i)
            {
                auto* colA = colliders[i];
                for (size_t j = i + 1; j < activeEntities.size(); ++j)
                {
                    auto* colB = colliders[j];
                    if (!colA || !colB) continue;

                    if (colA->mWorldOBB.Intersects(colB->mWorldOBB))
                    {
                        colA->bIsColliding = true;
                        colB->bIsColliding = true;
                    }
                }
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
        runN2();
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
                    }
                }
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