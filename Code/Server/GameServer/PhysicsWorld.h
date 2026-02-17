#pragma once
#include "Entity.h"

class World;
class BoxColliderComponent;
class TransformComponent;

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

struct AABB2D
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

struct StaticProxy
{
    Entity ColliderEntity;
    BoxColliderComponent* ColliderBox;
    AABB2D bounds;
	uint32 layerMask = 0;
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
	PhysicsWorld() = default;
    PhysicsWorld(World* world) : mWorld(world) { Initialize(); }

    void Initialize();

    void ClearStatic() { staticObjects.clear(); }
    void ClearNode() { nodes.clear(); }

    //void AddStaticOBB(Entity id, DirectX::BoundingOrientedBox obb, uint32 layerMask)
    //{
    //    StaticCollider c;
    //    c.id = id;
    //    c.layerMask = layerMask;
    //    c.type = StaticColliderType::OBB;
    //    c.obb = obb;
    //    mStatics.push_back(c);
    //}


public: //Query

    SweepHit SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius);
    void QueryStaticBVH(const AABB2D& query, std::vector<int>& outIndices);
    int BuildStaticBVHRecursive(
        std::vector<StaticProxy>& proxies,
        std::vector<BVHNode>& nodes,
        int start,
        int count);
public: // utils
    static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col);
    static void SetWorldOBB(BoundingOrientedBox obb,const TransformComponent* tr, BoxColliderComponent* col);
    
    
    static AABB2D BuildAABBFromOBB(const BoundingOrientedBox& obb);
    static AABB2D MergeAABB(const AABB2D& a, const AABB2D& b);
    static bool OverlapAABB(const AABB2D& a, const AABB2D& b);

public:
	int32 GetRootNodeIndex() const { return root; }
	std::vector<BVHNode>& GetBVHNodes() { return nodes; }
	std::vector<StaticProxy>& GetStaticProxies() { return staticObjects; }
	StaticProxy& GetStaticProxy(int index) { return staticObjects[index]; }
private:
	World* mWorld = nullptr;
    //  정적 월드 충돌 대상
    std::vector<StaticProxy> staticObjects;
    std::vector<BVHNode> nodes;
    int32                root{};
};

