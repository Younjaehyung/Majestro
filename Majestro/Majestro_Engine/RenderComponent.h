#pragma once
#include "Component.h"
#include "Shader.h"

class Mesh;
class Material;

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
	RenderComponent(shared_ptr<Mesh> mesh, vector<shared_ptr<Material>> materials) : mMesh(mesh) , mMaterials(materials) {}
	uint8 GetLayerIndex() { return mLayerIndex; }
	bool IsVisibility() { return mVisibility; }
	uint64 GetInstanceID();
public:
	bool mCheckFrustum = true;
	bool mCheckVisibilty = true;

	//자기 layer인덱스 확인
	uint8 mLayerIndex = 0;

	shared_ptr<Mesh> mMesh;
	vector<shared_ptr<Material>> mMaterials;





	bool mVisibility;
};

