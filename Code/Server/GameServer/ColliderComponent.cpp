#include "pch.h"
#include "ColliderComponent.h"

BoxColliderComponent::BoxColliderComponent()
{
	RebuildLocalOBB();
}



BoxColliderComponent::BoxColliderComponent(BoundingOrientedBox obb, Matrix matrix)
{
	// obb을 로컬 OBB로 저장하고, worldMatrix로 mWorldOBB를 즉시 계산
	mLocalOBB = obb;
	obb.BoundingOrientedBox::Transform(mWorldOBB, matrix);
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
}
