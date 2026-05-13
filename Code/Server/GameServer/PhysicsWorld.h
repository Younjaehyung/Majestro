#pragma once
#include "Entity.h"
#include "SpatialGrid2D.h"

class World;
class BoxColliderComponent;
class SphereColliderComponent;
class TransformComponent;
class CollisionMesh;
struct JoltTerrainState;

enum StaticColliderType
{
    OBB,
    AABB,
    CONVEX_HULL,
};

struct SweepHit
{
    bool hit = false;
    float distance = 0.0f;
    Entity colliderId = 0;
    Vector3 point{};
};

struct JoltStaticHit
{
    bool hit = false;
    Entity colliderId = 0;
    float distance = 0.0f;
    float fraction = 0.0f;
    Vector3 point{};
    Vector3 normal{};
};

struct AABB2D
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

enum class StaticProxyShape
{
    Box,
    Sphere
};

struct StaticProxy
{
    Entity ColliderEntity;
    BoxColliderComponent* ColliderBox;
    AABB2D bounds;

	uint32 layerMask = 0;
    StaticProxyShape shape = StaticProxyShape::Box;
    BoundingSphere sphere{};
    BoundingOrientedBox sphereBounds{};

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

class PhysicsWorld
{
public:

	PhysicsWorld();
    PhysicsWorld(World* world);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;


    void Initialize();
    void RebuildStaticBVH();
    bool SyncStaticBVHIfNeeded();
    size_t CountStaticCollisionEntities() const;

    void ClearStatic();
    void ClearNode() { nodes.clear(); root = -1; }

public: // Query
    SweepHit SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius);

    bool RayCastStatic(const Vector3& start, const Vector3& end, JoltStaticHit& outHit);

    bool QueryAimPoint(Entity shooter, const Vector3& cameraPos, const Vector3& cameraForward, float maxDistance, Vector3& outAimPoint);
    void QueryStaticBVH(const AABB2D& query, std::vector<int>& outIndices);
    void UpdateDynamicSpatialIndex();
    std::vector<Entity> FindNearbyEnemies(const Entity& entity, float radius, size_t maxCount);
    void GetMovableCollisionPairs(std::vector<std::pair<Entity, Entity>>& outPairs);
    int BuildStaticBVHRecursive(
        std::vector<StaticProxy>& proxies,
        std::vector<BVHNode>& nodes,
        int start,
        int count);

    
    float QueryHeightAtPosition(const Vector3& position);
    bool TryQueryTerrainHeight(const Vector3& position, float& outHeight) const;
    bool TryQueryTerrainHeightNear(const Vector3& position, float expectedHeight, float maxStepUp, float maxDropDown, float& outHeight) const;
    bool AddTerrainRayCastMesh(const CollisionMesh& mesh, const Matrix& worldMatrix);
    bool AddStaticCollisionMesh(Entity owner, const CollisionMesh& mesh, const Matrix& worldMatrix);
    bool CastMovingSphereAgainstStatic(const Vector3& start, const Vector3& end, float radius, JoltStaticHit& outHit) const;
    void OptimizeJoltStaticCollision();
    bool HasJoltTerrain() const;
public: // utils
    static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);
    static void UpdateWorldSphere(const TransformComponent* tr, SphereColliderComponent* col);
    static void SetWorldOBB(BoundingOrientedBox obb,const TransformComponent* tr, BoxColliderComponent* col);
    
    

    static AABB2D BuildAABBFromOBB(const BoundingOrientedBox& obb);
    static AABB2D BuildAABBFromSphere(const BoundingSphere& sphere);
    static AABB2D MergeAABB(const AABB2D& a, const AABB2D& b);
    static bool OverlapAABB(const AABB2D& a, const AABB2D& b);

public:
    int32 GetRootNodeIndex() const { return root; }
    std::vector<BVHNode>& GetBVHNodes() { return nodes; }
    std::vector<StaticProxy>& GetStaticProxies() { return staticObjects; }
    StaticProxy& GetStaticProxy(int index) { return staticObjects[index]; }

private:
    void RebuildEnemyGrid();
    void RebuildMovableGrid();
    bool IsDeadEnemy(Entity entity) const;

private:
    World* mWorld = nullptr;
    std::vector<StaticProxy> staticObjects;
    std::vector<BVHNode> nodes;

    int32                root = -1;

    // Jolt terrain raycast: owns static mesh bodies used for ground height ray queries.
    std::unique_ptr<JoltTerrainState> mJoltTerrain;


    SpatialGrid2D mEnemyGrid;
    SpatialGrid2D mMovableGrid;
};
