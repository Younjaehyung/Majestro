#pragma once
#include "Entity.h"

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
    float fraction = 0.0f;
    float distance = 0.0f;     // start로부터 거리
    Entity colliderId = 0;   // 어떤 정적 콜라이더에 맞았는지
    Vector3 point{};
    Vector3 normal{};
};

struct StaticCollider
{
    Entity id = 0;
    uint32 layerMask = 0;
    StaticColliderType type = StaticColliderType::OBB;
    BoundingBox aabb{};
    BoundingOrientedBox obb{};
};

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void ClearStatic();

    void AddStaticOBB(Entity id, DirectX::BoundingOrientedBox obb, uint32 layerMask)
    {
        StaticCollider c;
        c.id = id;
        c.layerMask = layerMask;
        c.type = StaticColliderType::OBB;
        c.obb = obb;
        mStatics.push_back(c);
    }

    SweepHit SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius);

    bool AddStaticCollisionMesh(Entity owner, const CollisionMesh& mesh, const Matrix& worldMatrix);
    bool CastMovingSphereAgainstStatic(const Vector3& start, const Vector3& end,
                                       float radius, JoltStaticHit& outHit) const;
    void OptimizeJoltStaticCollision();
    bool HasJoltStaticCollision() const;

private:
    std::vector<StaticCollider>       mStatics;
    std::unique_ptr<JoltTerrainState> mJoltTerrain;
};
