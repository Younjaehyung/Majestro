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

	obb.BoundingOrientedBox::Transform(mLocalOBB, matrix);
	mLocalOBB = obb;
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
