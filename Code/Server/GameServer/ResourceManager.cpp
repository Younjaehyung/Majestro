#include "pch.h"
#include "ResourceManager.h"
#include "HeightField.h"
#include "Texture.h"
#include "TerrainComponent.h"
#include "Mesh.h"
#include "FBX.h"

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::Initialize()
{
    LoadResources();
}


void ResourceManager::LoadResources()
{
	{  
		// Terrain HeightField
		//std::shared_ptr<HeightField> heightField = std::make_shared<HeightField>();
		//heightField->LoadHeightFieldFromPng16("../Resources/Terrain/heightfield.png"/*512, 512, 1.0f, 0.1f*/);
		//Add<HeightField>(L"TerrainHeightField", heightField);
		//auto heightField = std::make_shared<HeightField>();
		//heightField->LoadHeightFieldFromRaw16("../Resources/Texture/height.raw", 2048, 2048, true);
		//Add<HeightField>(L"TerrainHeightField", heightField);
    }
}

shared_ptr<FBX>& ResourceManager::LoadFBXMeshes(const wstring& path)
{
	shared_ptr<FBX> meshData = Get<FBX>(s2ws(filesystem::path(path).filename().stem().string()));
	if (meshData)
		return meshData;
	meshData = make_shared<FBX>();
	meshData->Load(path);
	meshData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(s2ws(filesystem::path(path).filename().stem().string()), meshData);

	return meshData;
}

