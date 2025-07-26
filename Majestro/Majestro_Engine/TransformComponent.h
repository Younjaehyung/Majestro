
class TransformComponent
{
public:

	Vec3 GetWorldPosition() { return mWorldPosition; }
	float GetBoundingSphereRadius() { return mBoundingSphere.Radius; }
public:
	BoundingSphere mBoundingSphere;
	Vec3 mWorldPosition;
};
	