#include "Entity.h"
#include "Component.h"

struct TransformParams {
	Matrix matWorld;
	Matrix matView;
	Matrix matProjection;
	Matrix matWV;
	Matrix matWVP;
	Matrix matViewInv;
};

class TransformComponent : public Component<TransformComponent>
{
public:

	Vec3 GetWorldPosition() { return mWorldPosition; }
	float GetBoundingSphereRadius() { return mBoundingSphere.Radius; }
	const Matrix& GetLocalToWorldMatrix() { return _matWorld; }

	void SetLocalScale(const Vec3& scale) { _localScale = scale; }
public:

	Vec3 _localPosition = {};
	Vec3 _localRotation = {};
	Vec3 _localScale = { 1.f, 1.f, 1.f };

	Matrix _matLocal = {};
	Matrix _matWorld = {};

	BoundingSphere mBoundingSphere;
	Vec3 mWorldPosition;

	Entity mParent;
	vector<Entity> mChild;
};
	