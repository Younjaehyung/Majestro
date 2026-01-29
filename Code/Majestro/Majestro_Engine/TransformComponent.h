#pragma once
#include "Entity.h"
#include "Component.h"

struct ObjectParams {
	Matrix MatWorld;
};

class TransformComponent : public Component<TransformComponent>
{
public:
	TransformComponent(){}
	TransformComponent(Vec3 position, Vec3 scale): mLocalPosition(position), mLocalScale(scale){}
	TransformComponent(Vec3 position): mLocalPosition(position){}


	Vec3 GetWorldPosition() { return mWorldPosition; }
	float GetBoundingSphereRadius() { return mBoundingSphere.Radius; }
	const Matrix& GetWorldMatrix() { return mWorldMatrix; }

	Vec3 GetRight() { return mWorldMatrix.Right(); }
	Vec3 GetUp() { return mWorldMatrix.Up(); }
	Vec3 GetLook() { return mWorldMatrix.Backward(); }

	void SetLocalScale(const Vec3& scale) { mLocalScale = scale; }

	void LookAt(const Vec3 dir);
	bool CloseEnough(const float& a, const float& b, const float& epsilon = std::numeric_limits<float>::epsilon());
	Vec3 DecomposeRotationMatrix(const Matrix& rotation);
	void FinalUpdate();
public:

	Vec3 mLocalPosition = {};
	Vec3 mLocalRotation = {};
	Vec3 mLocalScale = { 1.f, 1.f, 1.f };

	Matrix mLocalMatrix = {};
	Matrix mWorldMatrix = {};

	BoundingSphere mBoundingSphere;
	Vec3 mWorldPosition;

	Entity mParent;
	vector<Entity> mChild;
	bool mIsStatic = false;
public:
	uint8 mBufferIndex = 0;
};
	