#pragma once
#include "Component.h"
#include "TransformComponent.h"

class CollisionMesh;

class BoxColliderComponent : public Component<BoxColliderComponent>
{
public:
    BoxColliderComponent();
	// 수정 내용
	// BoundingOrientedBox 를 직접 받는 경우도 이후 UpdateWorldOBB 에서 mLocalOBB 를 기준으로 다시 변환한다.
	// mWorldOBB 만 채우면 mLocalOBB Orientation 이 기본값으로 남아 DirectXCollision 단위 쿼터니언 검사에 실패할 수 있다.
	BoxColliderComponent(BoundingOrientedBox obb) : mLocalOBB(obb), mWorldOBB(obb) {}
    BoxColliderComponent(BoundingOrientedBox obb, Matrix matrix);
    BoxColliderComponent(Vec3 half) : mHalfExtents(half) { RebuildLocalOBB(); }
    BoxColliderComponent(Vec3 half, Vec3 center);

    void SetHalfExtents(const Vec3& halfExtents);
    void SetCenter(const Vec3& center);
    void SetBox(const Vec3& halfExtents, const Vec3& center);
    void RebuildLocalOBB();

public:
    // 로컬 공간 기준 박스(OBB의 로컬 정의)
    // Center: 로컬 중심 오프셋
    // HalfExtents: 로컬 반길이 (x,y,z)
    Vec3 mCenter = Vec3(0, 0, 0);
    Vec3 mHalfExtents = Vec3(10.5f, 10.5f, 10.5f);

    // [옵션] 디버그 표시용
    bool bDebugDraw = true;
    bool bNoDepth = false; // true면 항상 보이게(Depth Test X)

    bool bIsColliding = false;
	bool bIsInitialized = false;

    bool bIsTrigger = false; // 겹침 판정만 수행하고 물리 응답은 적용하지 않는다.(InteractionSystem용)
    BoundingOrientedBox mLocalOBB{};
    BoundingOrientedBox mWorldOBB{};

    // Jolt terrain raycast: optional source mesh used only for ray based ground height queries.
    shared_ptr<CollisionMesh> mRayCastMesh = nullptr;
};

class SphereColliderComponent : public Component<SphereColliderComponent>
{
public:
    SphereColliderComponent() = default;
    SphereColliderComponent(float radius) : mLocalRadius(radius) {}

public:
    // 수정 내용
    float mLocalRadius = 50.0f;
    BoundingSphere mWorldSphere{};
    BoundingOrientedBox mWorldBounds{};
    Vec3 mWorldRadii = Vec3(50.0f, 50.0f, 50.0f);
    bool bIsColliding = false;
};
