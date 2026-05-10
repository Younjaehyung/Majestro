#pragma once

#include "pch.h"
#include "Component.h"

#include <unordered_set>

class Entity;

struct SpatialGridBounds2D
{
    float minX = 0.0f;
    float maxX = 0.0f;
    float minZ = 0.0f;
    float maxZ = 0.0f;
};

struct SpatialGridItem2D
{
    Entity entity;
    SpatialGridBounds2D bounds;
    Vec3 position = Vec3::Zero;
};

class SpatialGrid2D
{
public:
    void Clear();
    bool Empty() const;

    void Build(
        const std::vector<SpatialGridItem2D>& items,
        float minCellSize = 10.0f,
        size_t maxCellCount = 1'000'000);

    void QueryRadius(const Vec3& center, float radius, std::vector<Entity>& outEntities) const;
    void QueryAABB(const SpatialGridBounds2D& bounds, std::vector<Entity>& outEntities) const;
    void GetCandidatePairs(std::vector<std::pair<Entity, Entity>>& outPairs) const;

private:
    bool IsBuilt() const;
    size_t ToCellIndex(int cellX, int cellZ) const;
    int ClampIndex(int value, int minValue, int maxValue) const;
    void GetCellRange(const SpatialGridBounds2D& bounds, int& minCellX, int& maxCellX, int& minCellZ, int& maxCellZ) const;

private:
    std::vector<SpatialGridItem2D> mItems;
    std::vector<uint32_t> mCounts;
    std::vector<uint32_t> mOffsets;
    std::vector<uint32_t> mIndices;

    float mWorldMinX = 0.0f;
    float mWorldMaxX = 0.0f;
    float mWorldMinZ = 0.0f;
    float mWorldMaxZ = 0.0f;
    float mCellSize = 10.0f;
    float mInvCellSize = 0.1f;
    size_t mCellsX = 0;
    size_t mCellsZ = 0;
};
