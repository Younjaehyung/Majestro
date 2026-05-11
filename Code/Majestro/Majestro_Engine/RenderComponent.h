#pragma once
#include "Component.h"
#include "Shader.h"

class Mesh;
class Material;
class TransformComponent;

union InstanceID
{
	struct
	{
		uint32 MeshID;
		uint32 MaterialID;
	};
	uint64 ID;
};


class RenderComponent : public Component<RenderComponent>
{
public:
	RenderComponent();
	RenderComponent(shared_ptr<Mesh> mesh, vector<shared_ptr<Material>>& materials);
	uint8 GetLayerIndex() { return mLayerIndex; }
	bool IsVisibility() { return mVisibility; }
	uint64 GetInstanceID();
	void SetMesh(shared_ptr<Mesh> mesh);
	void SetLocalOBB(const Vec3& center, const Vec3& halfExtents);
	void UpdateWorldOBB(TransformComponent* transformComponent);
public:
	bool mCheckFrustum = true;
	bool mCheckVisibilty = true;
	bool mIsNotObject = false;

	//자기 layer인덱스 확인
	uint8 mLayerIndex = 0;

	shared_ptr<Mesh> mMesh;
	vector<shared_ptr<Material>> mMaterials;

	uint32	mObjectIndex{};
	bool mVisibility{true};

	Vec3 mObbCenter = Vec3(0.f, 0.f, 0.f);
	Vec3 mObbHalfExtents = Vec3(0.5f, 0.5f, 0.5f);

	BoundingOrientedBox mLocalOBB{};
	BoundingOrientedBox mWorldOBB{};
};

