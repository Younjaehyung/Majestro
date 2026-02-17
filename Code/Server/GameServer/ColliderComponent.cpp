#include "pch.h"
#include "ColliderComponent.h"

BoxColliderComponent::BoxColliderComponent()
{
	BoundingOrientedBox localOBB;
	localOBB.Center = mCenter;
	localOBB.Extents = mHalfExtents;
	localOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);

	localOBB.Transform(mLocalOBB, Matrix::Identity);
}

BoxColliderComponent::BoxColliderComponent(BoundingOrientedBox obb, Matrix matrix)
{
	// OBB의 로컬 정의 -> 월드 정의로 변환
	obb.BoundingOrientedBox::Transform(mLocalOBB, matrix);

}

BoxColliderComponent::BoxColliderComponent(Vec3 half, Vec3 center)
:mHalfExtents(half), mCenter(center)
{
	
	BoundingOrientedBox localOBB;
	localOBB.Center = XMFLOAT3(mCenter.x, mCenter.y, mCenter.z);
	localOBB.Extents = XMFLOAT3(mHalfExtents.x, mHalfExtents.y, mHalfExtents.z);
	localOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);

	localOBB.Transform(mLocalOBB, Matrix::Identity);
}
