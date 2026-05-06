#include "pch.h"
#include "PhysicsWorld.h"
#include "World.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "TagComponent.h"
#include "MovementComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"

void PhysicsWorld::Initialize()
{
    if (false == mWorld->HasComponentPool<StaticComponent>()) return;
    auto staticEntities = mWorld->GetEntitiesWithComponents<StaticComponent, TransformComponent, BoxColliderComponent>();

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

    nodes.reserve(staticObjects.size() * 2);
    root = BuildStaticBVHRecursive(staticObjects, nodes, 0, static_cast<int>(staticObjects.size()));
}

AABB2D PhysicsWorld::MergeAABB(const AABB2D& a, const AABB2D& b)
{
    return AABB2D{
        (std::min)(a.minX, b.minX),
        (std::max)(a.maxX, b.maxX),
        (std::min)(a.minZ, b.minZ),
        (std::max)(a.maxZ, b.maxZ)
    };
}

bool PhysicsWorld::OverlapAABB(const AABB2D& a, const AABB2D& b)
{
    if (a.maxX < b.minX || b.maxX < a.minX)
        return false;
    if (a.maxZ < b.minZ || b.maxZ < a.minZ)
        return false;
    return true;
}

SweepHit PhysicsWorld::SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius)
{
    SweepHit best;
    for (auto& collider : staticObjects)
    {
        SweepHit out{};

        BoundingOrientedBox expanded = collider.ColliderBox->mWorldOBB;
        expanded.Extents.x += radius;
        expanded.Extents.y += radius;
        expanded.Extents.z += radius;

        Vec3 s = start;
        Vec3 e = end;
        Vec3 dir = e - s;

        float segLen = XMVectorGetX(XMVector3Length(dir));
        if (segLen <= 1e-6f)
            return out;

        dir.Normalize();

        if (expanded.Contains(s) != ContainmentType::DISJOINT)
        {
            out.hit = true;
            out.distance = 0.0f;
            return out;
        }

        float dist = 0.0f;
        if (expanded.Intersects(s, dir, dist))
        {
            if (dist >= 0.0f && dist <= segLen)
            {
                out.hit = true;
                out.distance = dist;
            }
        }

        if (best.hit || out.distance < best.distance)
        {
            out.colliderId = collider.ColliderEntity;
            best = out;
        }
    }
    return best;
}

std::vector<Entity> PhysicsWorld::FindNearbyEnemies(const Entity& entity, float radius, size_t maxCount)
{
    std::vector<Entity> result;
    if (!entity.IsValid() || radius <= 0.0f || maxCount == 0)
        return result;
    if (mEnemyGrid.Empty())
        return result;

    const TransformComponent* selfTf = mWorld->GetComponent<TransformComponent>(entity);
    if (!selfTf)
        return result;

    std::vector<Entity> candidates;
    mEnemyGrid.QueryRadius(selfTf->mLocalPosition, radius, candidates);

    std::vector<std::pair<float, Entity>> sortedCandidates;
    sortedCandidates.reserve(candidates.size());

    for (const Entity& other : candidates)
    {
        if (other == entity || IsDeadEnemy(other))
            continue;

        const TransformComponent* otherTf = mWorld->GetComponent<TransformComponent>(other);
        if (!otherTf)
            continue;

        Vec3 delta = otherTf->mLocalPosition - selfTf->mLocalPosition;
        delta.y = 0.0f;
        sortedCandidates.push_back({ delta.LengthSquared(), other });
    }

    std::sort(sortedCandidates.begin(), sortedCandidates.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return lhs.first < rhs.first;
        });

    const size_t count = (std::min)(maxCount, sortedCandidates.size());
    result.reserve(count);
    for (size_t i = 0; i < count; ++i)
        result.push_back(sortedCandidates[i].second);

    return result;
}

void PhysicsWorld::UpdateDynamicSpatialIndex()
{
    RebuildEnemyGrid();
    RebuildMovableGrid();
}

void PhysicsWorld::GetMovableCollisionPairs(std::vector<std::pair<Entity, Entity>>& outPairs)
{
    if (mMovableGrid.Empty())
    {
        outPairs.clear();
        return;
    }

    mMovableGrid.GetCandidatePairs(outPairs);
}

void PhysicsWorld::QueryStaticBVH(const AABB2D& query, std::vector<int>& outIndices)
{
    if (root < 0) return;

    std::vector<int> stack;
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

        if (node.left >= 0) stack.push_back(node.left);
        if (node.right >= 0) stack.push_back(node.right);
    }
}

void PhysicsWorld::RebuildEnemyGrid()
{
    mEnemyGrid.Clear();

    if (!mWorld->HasComponentPool<EnemyMovementComponent>() ||
        !mWorld->HasComponentPool<TransformComponent>() ||
        !mWorld->HasComponentPool<BoxColliderComponent>())
    {
        return;
    }

    std::vector<SpatialGridItem2D> items;
    items.reserve(mWorld->GetEntitiesWithComponent<EnemyMovementComponent>().size());

    for (const Entity& entity : mWorld->GetEntitiesWithComponent<EnemyMovementComponent>())
    {
        TransformComponent* tf = mWorld->GetComponent<TransformComponent>(entity);
        BoxColliderComponent* col = mWorld->GetComponent<BoxColliderComponent>(entity);
        if (!tf || !col)
            continue;
        if (IsDeadEnemy(entity))
            continue;

        UpdateWorldOBB(tf, col);
        const AABB2D aabb = BuildAABBFromOBB(col->mWorldOBB);

        SpatialGridItem2D item;
        item.entity = entity;
        item.position = tf->mLocalPosition;
        item.bounds.minX = aabb.minX;
        item.bounds.maxX = aabb.maxX;
        item.bounds.minZ = aabb.minZ;
        item.bounds.maxZ = aabb.maxZ;
        items.push_back(item);
    }

    mEnemyGrid.Build(items);
}

void PhysicsWorld::RebuildMovableGrid()
{
    mMovableGrid.Clear();

    if (!mWorld->HasComponentPool<MovableComponent>() ||
        !mWorld->HasComponentPool<TransformComponent>() ||
        !mWorld->HasComponentPool<BoxColliderComponent>())
    {
        return;
    }

    std::vector<SpatialGridItem2D> items;
    items.reserve(mWorld->GetEntitiesWithComponent<MovableComponent>().size());

    for (const Entity& entity : mWorld->GetEntitiesWithComponent<MovableComponent>())
    {
        TransformComponent* tf = mWorld->GetComponent<TransformComponent>(entity);
        BoxColliderComponent* col = mWorld->GetComponent<BoxColliderComponent>(entity);
        if (!tf || !col)
            continue;
        if (IsDeadEnemy(entity))
            continue;

        UpdateWorldOBB(tf, col);
        const AABB2D aabb = BuildAABBFromOBB(col->mWorldOBB);

        SpatialGridItem2D item;
        item.entity = entity;
        item.position = tf->mLocalPosition;
        item.bounds.minX = aabb.minX;
        item.bounds.maxX = aabb.maxX;
        item.bounds.minZ = aabb.minZ;
        item.bounds.maxZ = aabb.maxZ;
        items.push_back(item);
    }

    mMovableGrid.Build(items);
}

bool PhysicsWorld::IsDeadEnemy(Entity entity) const
{
    if (!mWorld || !entity.IsValid())
        return false;
    if (!mWorld->HasComponent<EnemyComponent>(entity))
        return false;

    const HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity);
    return health && health->IsDead();
}

int PhysicsWorld::BuildStaticBVHRecursive(
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
        node.bounds = MergeAABB(node.bounds, proxies[start + i].bounds);

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

float PhysicsWorld::QueryHeightAtPosition(const Vector3& position)
{
    float bestHeight = -FLT_MAX;

    for (const auto& collider : staticObjects)
    {
        const BoundingOrientedBox& obb = collider.ColliderBox->mWorldOBB;

        const Vector3 obbCenter = obb.Center;
        const Vector3 obbUp = Vector3(obb.Orientation.x, obb.Orientation.y, obb.Orientation.z);
        const float distance = (position - obbCenter).Dot(obbUp);

        if (distance >= 0)
        {
            const float height = obbCenter.y + distance;
            if (height > bestHeight)
                bestHeight = height;
        }
    }
    return bestHeight;
}

void PhysicsWorld::UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col)
{
    col->mLocalOBB.BoundingOrientedBox::Transform(col->mWorldOBB, tr->mWorldMatrix);
}

void PhysicsWorld::SetWorldOBB(BoundingOrientedBox obb, const TransformComponent* tr, BoxColliderComponent* col)
{
    obb.BoundingOrientedBox::Transform(col->mWorldOBB, tr->mWorldMatrix);
}

AABB2D PhysicsWorld::BuildAABBFromOBB(const BoundingOrientedBox& obb)
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
