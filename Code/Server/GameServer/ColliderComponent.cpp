#include "pch.h"
#include "ColliderComponent.h"

BoxColliderComponent::BoxColliderComponent(BoundingOrientedBox obb, Matrix matrix) 
{
	// OBB의 로컬 정의 -> 월드 정의로 변환
	obb.BoundingOrientedBox::Transform(mWorldOBB, matrix);

}
