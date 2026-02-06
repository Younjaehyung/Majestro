#include "pch.h"
#include "Engine.h"
#include "World.h"
#include "ResourceManager.h"
#include "CollisionSystem.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "MovementComponent.h"

static void UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col)
{
    Matrix worldMat = tr->mWorldMatrix;

    Vec3 scale;
    Quaternion rot;
    Vec3 trans;

    if (!worldMat.Decompose(scale, rot, trans))
        return;


    Vec3 localCenter = col->mCenter; 
    Vec3 worldCenter = trans + Vector3::Transform(localCenter, rot);

    col->mWorldOBB.Center = worldCenter;


    Vec3 originalExtents = col->mHalfExtents;
    col->mWorldOBB.Extents = originalExtents * scale;

    col->mWorldOBB.Orientation = rot;
}

CollisionSystem::CollisionSystem(World* world) : System(world)
{

}

void CollisionSystem::Update(float dt) {

    auto entities = mWorld->GetEntitiesWithComponents<TransformComponent, BoxColliderComponent>();

    // (B) 월드 OBB 갱신
    for (auto e : entities)
    {
        auto* tr = mWorld->GetComponent<TransformComponent>(e);
        auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
        if (col) col->bIsColliding = false;
        if (!tr || !col) continue;

        UpdateWorldOBB(tr, col);
    }
}