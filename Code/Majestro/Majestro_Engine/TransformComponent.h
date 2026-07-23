#pragma once
#include "Entity.h"
#include "Component.h"

class TransformComponent : public Component<TransformComponent>
{
public:
	TransformComponent(){}
	TransformComponent(Vec3 position, Vec3 scale): mLocalPosition(position), mLocalScale(scale){}
	TransformComponent(Vec3 position): mLocalPosition(position){}


	Vec3 GetWorldPosition() { return mWorldPosition; }
	float GetBoundingSphereRadius() { return mBoundingSphere.Radius; }
	const Matrix& GetWorldMatrix() { return mWorldMatrix; }
	uint64 GetLocalRevision() const { return mLocalRevision; }
	uint64 GetWorldRevision() const { return mWorldRevision; }

	Vec3 GetRight() { return mWorldMatrix.Right(); }
	Vec3 GetUp() { return mWorldMatrix.Up(); }
	Vec3 GetLook() { return mWorldMatrix.Backward(); }

	void SetLocalPosition(const Vec3& position) { mLocalPosition = position; }
	void SetLocalRotationE(const Vec3& rotation) { mLocalRotationE = rotation; }
	void SetLocalScale(const Vec3& scale) { mLocalScale = scale; }
public: 
	// Position



	// Scale
	

	// Rotate
	void UpdateRotationQuaternionFromEuler(Vec3 rotation); // 외부 Euler각 -> 쿼터니언 변환
	void UpdateRotationQuaternionFromEuler();	// 내부 Euler각 -> 쿼터니언 변환
	void UpdateRotationEulerFromQuaternion(); // 쿼터니언 -> Euler각 변환
	


	//Utils
	void LookAt(const Vec3 dir);
	bool CloseEnough(const float& a, const float& b, const float& epsilon = std::numeric_limits<float>::epsilon());
	Vec3 DecomposeRotationMatrix(const Matrix& rotation);

	// Update
	Matrix FinalUpdate(const Matrix& parentMatrix = Matrix(),uint64 parentWorldRevision = 0);
public:

	Vec3 mLocalPosition = {};
	Vec3 mLocalRotationE = {}; // Euler Degrees	(에디터 용)
	Vec3 mLocalScale = { 1.f, 1.f, 1.f };
	Vec3 mMovingVector;

	Vec3 mLocalRotationR = {}; // Radians
	Quaternion mLocalRotationQ = {}; // Quaternion


	Matrix mLocalMatTranslation = {};
	Matrix mLocalMatRotation = {};
	Matrix mLocalMatScale = {};
	

	Matrix mLocalMatrix = {};
	Matrix mWorldMatrix = {};

	BoundingSphere mBoundingSphere;
	Vec3 mWorldPosition;

	Entity mParent;
	vector<Entity> mChild;
	bool mIsStatic = false;

private:
	// public local 값을 직접 대입하는 기존 코드도 안전하게 감지하기 위한 마지막 계산 상태
	Vec3 mCachedLocalPosition{};
	Vec3 mCachedLocalRotationE{};
	Vec3 mCachedLocalScale{ 1.f, 1.f, 1.f };
	Entity mCachedParent;
	uint64 mCachedParentWorldRevision = 0;
	uint64 mLocalRevision = 0;
	uint64 mWorldRevision = 0;
	bool mLocalStateInitialized = false;
	bool mWorldStateInitialized = false;

public:
	uint8 mBufferIndex = 0;
};
