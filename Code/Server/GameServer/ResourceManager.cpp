#include "pch.h"
#include "ResourceManager.h"
#include "HeightField.h"
#include "Texture.h"
#include "TerrainComponent.h"
#include "Mesh.h"
#include "NavMeshLoader.h"
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


LevelImportData ResourceManager::LoadResourceJson(const std::wstring& path)
{
	std::string jsonPath = ws2s(path);
	std::ifstream ifs(jsonPath);
	if (!ifs.is_open())
		throw std::runtime_error("Failed to open json: " + jsonPath);

	json root;
	ifs >> root;

	LevelImportData out{};
	out.levelName = GetString(root, "level_name");

	if (root.contains("actual_export_root"))
		out.actualExportRoot = root["actual_export_root"].get<std::string>();

	float positionUnitScale = 1.0f;
	if (root.contains("units"))
	{
		const std::string units = root["units"].get<std::string>();
		if (units == "cm")
		{

			positionUnitScale = 1.0f;
		}
		else if (units == "m")
		{

			positionUnitScale = 1.0f;
		}
	}

	const auto& actors = Require(root, "actors");
	if (!actors.is_array())
		throw std::runtime_error("JSON 'actors' is not an array");

	for (const auto& a : actors)
	{
		const std::string actorName = GetString(a, "name");
		const std::string actorPath = GetString(a, "path");

		const auto& comps = Require(a, "static_mesh_components");
		if (!comps.is_array())
			throw std::runtime_error("JSON components is not an array");

		for (const auto& c : comps)
		{
			const std::string compName = GetString(c, "component_name");
			const std::string meshAsset = GetString(c, "static_mesh_asset");
			const std::string fbxPath = (c.contains("fbx") && !c["fbx"].is_null()) ? c["fbx"].get<std::string>() : "";

			if (c.contains("instances") && c["instances"].is_array())
			{
				const auto& instances_json = c["instances"];
				out.instances.reserve(out.instances.size() + instances_json.size());

				for (const auto& inst_j : instances_json)
				{
					MeshInstance insts{};
					insts.actorName = actorName;
					insts.actorPath = actorPath;
					insts.componentName = compName;
					insts.staticMeshAsset = meshAsset;
					insts.fbx = fbxPath;

					const auto& dx = Require(inst_j, "dx");

					insts.world = ParseDxTransform(dx);
					//const auto& ue = Require(inst_j, "ue");
					//insts.ue = ParseUETransform(inst_j, positionUnitScale);

					{
						insts.worldMtx = BuildWorldMatrix_RowMajor(insts.world, false);// *Matrix::CreateTranslation(Vec3(-9493.f, -620.f, 15647.0f));
						//insts.worldMtx = ConvertTransform(insts.ue) * 
						//	Matrix::CreateTranslation(Vec3(-9493.f, -472.0f, 15647.0f)); // UE to DX 변환
						//
					}

					out.instances.push_back(std::move(insts));
				}
			}
			else
			{
				MeshInstance inst{};
				inst.actorName = actorName;
				inst.actorPath = actorPath;
				inst.componentName = compName;
				inst.staticMeshAsset = meshAsset;
				inst.fbx = fbxPath;

				const auto& cwt = Require(c, "component_world_transform");
				const auto& dx = Require(cwt, "dx");
				inst.world = ParseDxTransform(dx);
				//const auto& ue = Require(cwt, "ue");
				//inst.ue = ParseUETransform(ue, positionUnitScale);

				{
					inst.worldMtx = BuildWorldMatrix_RowMajor(inst.world, false);// *Matrix::CreateTranslation(Vec3(-9493.f, -620.f, 15647.0f));



				}


				out.instances.push_back(std::move(inst));
			}
		}
	}

	return out;
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

