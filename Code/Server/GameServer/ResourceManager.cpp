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
	out.levelName = GetOptionalString(root, "level_name");

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

	const auto& actors = RequireJson(root, "actors");
	if (!actors.is_array())
		throw std::runtime_error("JSON 'actors' is not an array");

	for (const auto& a : actors)
	{
		const std::string actorName = GetOptionalString(a, "name");
		const std::string actorPath = GetOptionalString(a, "path");

		const auto& comps = RequireJson(a, "static_mesh_components");
		if (!comps.is_array())
			throw std::runtime_error("JSON components is not an array");

		for (const auto& c : comps)
		{
			const std::string compName = GetOptionalString(c, "component_name");
			const std::string meshAsset = GetOptionalString(c, "static_mesh_asset");
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

					const auto& dx = RequireJson(inst_j, "dx");

					insts.world = ParseDxTransform(dx);
					//const auto& ue = Require(inst_j, "ue");
					//insts.ue = ParseUETransform(inst_j, positionUnitScale);

					{
						insts.worldMtx = BuildWorldMatrix_RowMajor(insts.world, false);// *Matrix::CreateTranslation(Vec3(-9493.f, -620.f, 15647.0f));
						//insts.worldMtx = ConvertTransform(insts.ue) * 
						//	Matrix::CreateTranslation(Vec3(-9493.f, -472.0f, 15647.0f)); // UE to DX 변환
						
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

				const auto& cwt = RequireJson(c, "component_world_transform");
				const auto& dx = RequireJson(cwt, "dx");
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

shared_ptr<PayloadPathData> ResourceManager::LoadPayloadPathJson(const std::wstring& path)
{
	shared_ptr<PayloadPathData> loadData = Get<PayloadPathData>(s2ws(filesystem::path(path).filename().stem().string()));
	if (loadData)
		return loadData;
	loadData = make_shared<PayloadPathData>();
	loadData->LoadFromJsonFile(path);
	loadData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(loadData->GetName(), loadData);
	return loadData;
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
	{
		// NavMesh - FirstGame
		std::shared_ptr<NavMesh> navMesh = std::make_shared<NavMesh>();
		navMesh->Load("../Resources/NavMesh/all_tiles_navmesh.bin");
		Add<NavMesh>(L"NavMesh_FirstGame", navMesh);

		LoadPayloadPathJson(L"../Resources/Json/BP_Payroad_path_C_2_PayloadPath.json");
	}
	{
		// NavMesh - SecondGame
		std::shared_ptr<NavMesh> navMesh = std::make_shared<NavMesh>();
		navMesh->Load("../Resources/NavMesh/MapDesert_navmesh.bin");
		Add<NavMesh>(L"NavMesh_SecondGame", navMesh);
	}
	{
		// NavMesh - ThirdGame
		std::shared_ptr<NavMesh> navMesh = std::make_shared<NavMesh>();
		navMesh->Load("../Resources/NavMesh/Map003_navmesh.bin");
		Add<NavMesh>(L"NavMesh_ThirdGame", navMesh);
	}
}

shared_ptr<FBX> ResourceManager::LoadFBXMeshes(const wstring& path, const wstring& prefix)
{
	wstring stem = s2ws(filesystem::path(path).filename().stem().string());
	wstring key = MakeKey(prefix, stem);

	shared_ptr<FBX> meshData = Get<FBX>(key);
	if (meshData)
		return meshData;
	meshData = make_shared<FBX>();
	meshData->SetNamespace(prefix);
	meshData->Load(path);
	meshData->SetName(key);
	Add(key, meshData);

	return meshData;
}

void ResourceManager::DebugCheckKeyCollision(uint8 objectType, const wstring& key, const wstring& path)
{
#ifdef _DEBUG
	
	static std::map<std::pair<uint8, wstring>, wstring> sKeyPath;
	auto pk = std::make_pair(objectType, key);
	auto it = sKeyPath.find(pk);
	if (it == sKeyPath.end())
		sKeyPath.emplace(pk, path);
	else if (it->second != path)
		std::cout << "[ResLoad][COLLISION] key=" << ws2s(key)
		<< " old=" << ws2s(it->second) << " new=" << ws2s(path) << "\n";
#endif
}

shared_ptr<Mesh> ResourceManager::LoadMCubeMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"MCube");
	if (findMesh)
		return findMesh;

	// 수정 내용
	// Collision JSON 은 1x1x1 큐브를 배치한다는 전제로 export 된다.
	// 로컬 큐브 반지름을 0.5 로 둬야 JSON scale 이 곧 월드 충돌체 크기가 된다.
	float w2 = 0.5f;
	float h2 = 0.5f;
	float d2 = 0.5f;

	vector<Vertex> vec(24);

	// �ո�
	vec[0] = Vertex(Vec3(-w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	// �޸�
	vec[4] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[5] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[6] = Vertex(Vec3(+w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[7] = Vertex(Vec3(-w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	// ����
	vec[8] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[9] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[10] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[11] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	// �Ʒ���
	vec[12] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[13] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[14] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[15] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	// ���ʸ�
	vec[16] = Vertex(Vec3(-w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[17] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[18] = Vertex(Vec3(-w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[19] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	// �����ʸ�
	vec[20] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[21] = Vertex(Vec3(+w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[22] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[23] = Vertex(Vec3(+w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));

	vector<uint32> idx(36);

	// �ո�
	idx[0] = 0; idx[1] = 1; idx[2] = 2;
	idx[3] = 0; idx[4] = 2; idx[5] = 3;
	// �޸�
	idx[6] = 4; idx[7] = 5; idx[8] = 6;
	idx[9] = 4; idx[10] = 6; idx[11] = 7;
	// ����
	idx[12] = 8; idx[13] = 9; idx[14] = 10;
	idx[15] = 8; idx[16] = 10; idx[17] = 11;
	// �Ʒ���
	idx[18] = 12; idx[19] = 13; idx[20] = 14;
	idx[21] = 12; idx[22] = 14; idx[23] = 15;
	// ���ʸ�
	idx[24] = 16; idx[25] = 17; idx[26] = 18;
	idx[27] = 16; idx[28] = 18; idx[29] = 19;
	// �����ʸ�
	idx[30] = 20; idx[31] = 21; idx[32] = 22;
	idx[33] = 20; idx[34] = 22; idx[35] = 23;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->CreateVertexBuffer(vec);
	mesh->CreateIndexBuffer(idx);
	mesh->SetName(L"MCube");
	Add(L"MCube", mesh);

	return mesh;
}
