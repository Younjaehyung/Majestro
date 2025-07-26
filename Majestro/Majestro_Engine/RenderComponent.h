#pragma once
#include "Component.h"
#include "Shader.h"

class Mesh;
class Material;

union InstanceID
{
	struct
	{
		uint32 meshID;
		uint32 materialID;
	};
	uint64 id;
};


class RenderComponent
{
public:
	uint8 GetLayerIndex() { return _layerIndex; }
	bool IsVisibility() { return mVisibility; }
	uint64 GetInstanceID();
public:
	bool _checkFrustum = true;
	bool checkVisibilty = true;

	//자기 layer인덱스 확인
	uint8 _layerIndex = 0;

	Mesh* mMesh;
	Material* mMaterial;

	vector<shared_ptr<Material>> mMaterials;

	bool mVisibility;
};

