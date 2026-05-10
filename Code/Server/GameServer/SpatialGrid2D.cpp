#include "pch.h"
#include "Entity.h"
#include "SpatialGrid2D.h"

void SpatialGrid2D::Clear()
{
    mItems.clear();
    mCounts.clear();
    mOffsets.clear();
    mIndices.clear();

    mWorldMinX = 0.0f;
    mWorldMaxX = 0.0f;
    mWorldMinZ = 0.0f;
    mWorldMaxZ = 0.0f;
    mCellSize = 10.0f;
    mInvCellSize = 0.1f;
    mCellsX = 0;
    mCellsZ = 0;
}

bool SpatialGrid2D::Empty() const
{
    return mItems.empty();
}

void SpatialGrid2D::Build(
    const std::vector<SpatialGridItem2D>& items,
    float minCellSize,
    size_t maxCellCount)
{
    Clear();
    if (items.empty())
        return;

    mItems = items;

    float maxExtent = 0.0f;
    mWorldMinX = (std::numeric_limits<float>::max)();
    mWorldMaxX = std::numeric_limits<float>::lowest();
    mWorldMinZ = (std::numeric_limits<float>::max)();
    mWorldMaxZ = std::numeric_limits<float>::lowest();

    for (const SpatialGridItem2D& item : mItems)
    {
        mWorldMinX = (std::min)(mWorldMinX, item.bounds.minX);
        mWorldMaxX = (std::max)(mWorldMaxX, item.bounds.maxX);
        mWorldMinZ = (std::min)(mWorldMinZ, item.bounds.minZ);
        mWorldMaxZ = (std::max)(mWorldMaxZ, item.bounds.maxZ);

        const float extentX = (item.bounds.maxX - item.bounds.minX) * 0.5f;
        const float extentZ = (item.bounds.maxZ - item.bounds.minZ) * 0.5f;
        maxExtent = (std::max)(maxExtent, (std::max)(extentX, extentZ));
    }

    mCellSize = (std::max)(minCellSize, maxExtent * 2.0f);
    mInvCellSize = 1.0f / mCellSize;

    const float width = mWorldMaxX - mWorldMinX;
    const float depth = mWorldMaxZ - mWorldMinZ;

    mCellsX = (std::max<size_t>)(1, static_cast<size_t>(std::floor(width / mCellSize)) + 1);
    mCellsZ = (std::max<size_t>)(1, static_cast<size_t>(std::floor(depth / mCellSize)) + 1);

    const size_t cellCount = mCellsX * mCellsZ;
    if (cellCount == 0 || cellCount > maxCellCount)
    {
        Clear();
        return;
    }

    mCounts.assign(cellCount, 0);

    for (size_t i = 0; i < mItems.size(); ++i)
    {
        int minCellX = 0;
        int maxCellX = 0;
        int minCellZ = 0;
        int maxCellZ = 0;
        GetCellRange(mItems[i].bounds, minCellX, maxCellX, minCellZ, maxCellZ);

        for (int z = minCellZ; z <= maxCellZ; ++z)
        {
            for (int x = minCellX; x <= maxCellX; ++x)
            {
                ++mCounts[ToCellIndex(x, z)];
            }
        }
    }

    mOffsets.assign(cellCount + 1, 0);
    for (size_t i = 0; i < cellCount; ++i)
        mOffsets[i + 1] = mOffsets[i] + mCounts[i];

    std::vector<uint32_t> cursor = mOffsets;
    mIndices.assign(mOffsets.back(), 0);

    for (size_t i = 0; i < mItems.size(); ++i)
    {
        int minCellX = 0;
        int maxCellX = 0;
        int minCellZ = 0;
        int maxCellZ = 0;
        GetCellRange(mItems[i].bounds, minCellX, maxCellX, minCellZ, maxCellZ);

        for (int z = minCellZ; z <= maxCellZ; ++z)
        {
            for (int x = minCellX; x <= maxCellX; ++x)
            {
                const size_t cellIndex = ToCellIndex(x, z);
                mIndices[cursor[cellIndex]++] = static_cast<uint32_t>(i);
            }
        }
    }
}

void SpatialGrid2D::QueryRadius(const Vec3& center, float radius, std::vector<Entity>& outEntities) const
{
    outEntities.clear();
    if (!IsBuilt() || radius <= 0.0f)
        return;

    const SpatialGridBounds2D queryBounds{
        center.x - radius,
        center.x + radius,
        center.z - radius,
        center.z + radius
    };

    int minCellX = 0;
    int maxCellX = 0;
    int minCellZ = 0;
    int maxCellZ = 0;
    GetCellRange(queryBounds, minCellX, maxCellX, minCellZ, maxCellZ);

    std::unordered_set<EntityID> visited;
    const float radiusSq = radius * radius;

    for (int z = minCellZ; z <= maxCellZ; ++z)
    {
        for (int x = minCellX; x <= maxCellX; ++x)
        {
            const size_t cellIndex = ToCellIndex(x, z);
            const uint32_t start = mOffsets[cellIndex];
            const uint32_t end = mOffsets[cellIndex + 1];

            for (uint32_t i = start; i < end; ++i)
            {
                const SpatialGridItem2D& item = mItems[mIndices[i]];
                if (!item.entity.IsValid())
                    continue;
                if (!visited.insert(item.entity.GetID()).second)
                    continue;

                Vec3 delta = item.position - center;
                delta.y = 0.0f;
                if (delta.LengthSquared() <= radiusSq)
                    outEntities.push_back(item.entity);
            }
        }
    }
}

void SpatialGrid2D::QueryAABB(const SpatialGridBounds2D& bounds, std::vector<Entity>& outEntities) const
{
    outEntities.clear();
    if (!IsBuilt())
        return;

    int minCellX = 0;
    int maxCellX = 0;
    int minCellZ = 0;
    int maxCellZ = 0;
    GetCellRange(bounds, minCellX, maxCellX, minCellZ, maxCellZ);

    std::unordered_set<EntityID> visited;

    for (int z = minCellZ; z <= maxCellZ; ++z)
    {
        for (int x = minCellX; x <= maxCellX; ++x)
        {
            const size_t cellIndex = ToCellIndex(x, z);
            const uint32_t start = mOffsets[cellIndex];
            const uint32_t end = mOffsets[cellIndex + 1];

            for (uint32_t i = start; i < end; ++i)
            {
                const SpatialGridItem2D& item = mItems[mIndices[i]];
                if (!item.entity.IsValid())
                    continue;
                if (!visited.insert(item.entity.GetID()).second)
                    continue;

                const bool overlapX = !(item.bounds.maxX < bounds.minX || bounds.maxX < item.bounds.minX);
                const bool overlapZ = !(item.bounds.maxZ < bounds.minZ || bounds.maxZ < item.bounds.minZ);
                if (overlapX && overlapZ)
                    outEntities.push_back(item.entity);
            }
        }
    }
}

void SpatialGrid2D::GetCandidatePairs(std::vector<std::pair<Entity, Entity>>& outPairs) const
{
    outPairs.clear();
    if (!IsBuilt())
        return;

    std::unordered_set<uint64_t> checkedPairs;

    const size_t cellCount = mCellsX * mCellsZ;
    for (size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex)
    {
        const uint32_t start = mOffsets[cellIndex];
        const uint32_t end = mOffsets[cellIndex + 1];

        for (uint32_t a = start; a < end; ++a)
        {
            const Entity entityA = mItems[mIndices[a]].entity;
            if (!entityA.IsValid())
                continue;

            for (uint32_t b = a + 1; b < end; ++b)
            {
                const Entity entityB = mItems[mIndices[b]].entity;
                if (!entityB.IsValid() || entityA == entityB)
                    continue;

                const EntityID idA = entityA.GetID();
                const EntityID idB = entityB.GetID();
                const uint64_t key = idA < idB
                    ? (static_cast<uint64_t>(idA) << 32) | idB
                    : (static_cast<uint64_t>(idB) << 32) | idA;

                if (!checkedPairs.insert(key).second)
                    continue;

                outPairs.emplace_back(
                    idA < idB ? entityA : entityB,
                    idA < idB ? entityB : entityA);
            }
        }
    }
}

bool SpatialGrid2D::IsBuilt() const
{
    return !mItems.empty() && !mOffsets.empty() && mCellsX > 0 && mCellsZ > 0;
}

size_t SpatialGrid2D::ToCellIndex(int cellX, int cellZ) const
{
    return static_cast<size_t>(cellZ) * mCellsX + static_cast<size_t>(cellX);
}

int SpatialGrid2D::ClampIndex(int value, int minValue, int maxValue) const
{
    return (std::max)(minValue, (std::min)(value, maxValue));
}

void SpatialGrid2D::GetCellRange(
    const SpatialGridBounds2D& bounds,
    int& minCellX,
    int& maxCellX,
    int& minCellZ,
    int& maxCellZ) const
{
    minCellX = static_cast<int>(std::floor((bounds.minX - mWorldMinX) * mInvCellSize));
    maxCellX = static_cast<int>(std::floor((bounds.maxX - mWorldMinX) * mInvCellSize));
    minCellZ = static_cast<int>(std::floor((bounds.minZ - mWorldMinZ) * mInvCellSize));
    maxCellZ = static_cast<int>(std::floor((bounds.maxZ - mWorldMinZ) * mInvCellSize));

    minCellX = ClampIndex(minCellX, 0, static_cast<int>(mCellsX) - 1);
    maxCellX = ClampIndex(maxCellX, 0, static_cast<int>(mCellsX) - 1);
    minCellZ = ClampIndex(minCellZ, 0, static_cast<int>(mCellsZ) - 1);
    maxCellZ = ClampIndex(maxCellZ, 0, static_cast<int>(mCellsZ) - 1);
}
