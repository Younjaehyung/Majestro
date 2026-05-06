#include "pch.h"
#include "ColliderComponent.h"

BoxColliderComponent::BoxColliderComponent()
{
	RebuildLocalOBB();
}



BoxColliderComponent::BoxColliderComponent(BoundingOrientedBox obb, Matrix matrix)
{
	// 수정 내용
	// obb 를 로컬 OBB 로 저장하고 mWorldOBB 는 안전한 기본값으로 둔다.
	// worldMatrix 에 반사나 음수 스케일이 섞인 경우 DirectX 기본 Transform 이 단위 쿼터니언을 깨뜨릴 수 있으므로
	// 실제 월드 OBB 는 PhysicsWorld UpdateWorldOBB 의 안전 변환 경로에서 계산한다.
	(void)matrix;
	mLocalOBB = obb;
	mWorldOBB = mLocalOBB;
}

BoxColliderComponent::BoxColliderComponent(Vec3 half, Vec3 center)
:mHalfExtents(half), mCenter(center)
{
	RebuildLocalOBB();
}

void BoxColliderComponent::SetHalfExtents(const Vec3& halfExtents)
{
	mHalfExtents = halfExtents;
	RebuildLocalOBB();
}

void BoxColliderComponent::SetCenter(const Vec3& center)
{
	mCenter = center;
	RebuildLocalOBB();
}

void BoxColliderComponent::SetBox(const Vec3& halfExtents, const Vec3& center)
{
	mHalfExtents = halfExtents;
	mCenter = center;
	RebuildLocalOBB();
}

void BoxColliderComponent::RebuildLocalOBB()
{
	
	BoundingOrientedBox localOBB;
	localOBB.Center = XMFLOAT3(mCenter.x, mCenter.y, mCenter.z);
	localOBB.Extents = XMFLOAT3(mHalfExtents.x, mHalfExtents.y, mHalfExtents.z);
	localOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);

	localOBB.Transform(mLocalOBB, Matrix::Identity);
	// 수정 내용
	// 월드 변환 전에도 mWorldOBB Orientation 이 단위 쿼터니언을 유지하도록 안전한 기본값을 맞춘다.
	// 실제 월드 위치와 회전은 PhysicsWorld UpdateWorldOBB 에서 TransformComponent 기준으로 다시 계산된다.
	mWorldOBB = mLocalOBB;
}
