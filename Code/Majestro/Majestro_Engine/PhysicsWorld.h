#pragma once
#include "Entity.h"

enum StaticColliderType
{
    OBB,
    AABB,
    CONVEX_HULL,

};

struct SweepHit
{
    bool hit = false;
    float distance = 0.0f;     // start로부터 거리
    Entity colliderId = 0;   // 어떤 정적 콜라이더에 맞았는지
    Vector3 point{};
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

    void ClearStatic() { mStatics.clear(); }

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

private:
    // [이동] 정적 월드 충돌 대상은 CollisionSystem이 아니라 PhysicsWorld가 소유
    std::vector<StaticCollider> mStatics;
};

