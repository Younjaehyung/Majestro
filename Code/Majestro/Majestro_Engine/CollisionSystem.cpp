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