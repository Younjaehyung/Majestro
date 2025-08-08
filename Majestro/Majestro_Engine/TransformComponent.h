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


	Vec3 GetWorldPosition() { return mWorldPosition; }
	float GetBoundingSphereRadius() { return mBoundingSphere.Radius; }
	const Matrix& GetLocalToWorldMatrix() { return mWorldMatrix; }

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

public:
	uint8 mBufferIndex = 0;
};
	