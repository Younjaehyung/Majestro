#include "pch.h"
#include "ResourceManager.h"
#include "HeightField.h"
#include "Texture.h"


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
	{   // Terrain HeightField
		std::shared_ptr<HeightField> heightField = std::make_shared<HeightField>();
		heightField->LoadHeightFieldFromPng16("../Resources/Terrain/heightfield.png", 512, 512, 1.0f, 0.1f);
		Add<HeightField>(L"TerrainHeightField", heightField);
    }
}

