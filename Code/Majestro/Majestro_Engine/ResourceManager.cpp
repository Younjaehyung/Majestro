#include "pch.h"
#include "ResourceManager.h"
#include "Engine.h"
#include "RenderManager.h"
#include "RootSignature.h"
#include "FBXData.h"



void ResourceManager::Initialize()
{
	CreateDefaultRootSignature();
	CreateDefaultShader();
	CreateDefaultMaterial();

	LoadRectangleMesh();
	LoadSphereMesh();

	LoadWireCubeMesh();
	
}



shared_ptr<Mesh> ResourceManager::LoadPointMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Point");
	if (findMesh)
		return findMesh;

	vector<Vertex> vec(1);
	vec[0] = Vertex(Vec3(0, 0, 0), Vec2(0.5f, 0.5f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));

	vector<uint32> idx(1);
	idx[0] = 0;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Point", mesh);

	return mesh;
}


shared_ptr<Mesh> ResourceManager::LoadTerrainMesh(int32 sizeX, int32 sizeZ)
{

	vector<Vertex> vec;

	for (int32 z = 0; z < sizeZ + 1; z++)
	{
		for (int32 x = 0; x < sizeX + 1; x++)
		{
			Vertex vtx;
			vtx.pos = Vec3(static_cast<float>(x), 0, static_cast<float>(z));
			vtx.uv = Vec2(static_cast<float>(x), static_cast<float>(sizeZ - z));
			vtx.normal = Vec3(0.f, 1.f, 0.f);
			vtx.tangent = Vec3(1.f, 0.f, 0.f);

			vec.push_back(vtx);
		}
	}

	vector<uint32> idx;

	for (int32 z = 0; z < sizeZ; z++)
	{
		for (int32 x = 0; x < sizeX; x++)
		{
			//  [0]
			//   |	\
			//  [2] - [1]
			idx.push_back((sizeX + 1) * (z + 1) + (x));
			idx.push_back((sizeX + 1) * (z)+(x + 1));
			idx.push_back((sizeX + 1) * (z)+(x));
			//  [1] - [2]
			//   	\  |
			//		  [0]
			idx.push_back((sizeX + 1) * (z)+(x + 1));
			idx.push_back((sizeX + 1) * (z + 1) + (x));
			idx.push_back((sizeX + 1) * (z + 1) + (x + 1));
		}
	}

	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Terrain");
	if (findMesh)
	{
		findMesh->Init(vec, idx);
		return findMesh;
	}

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Terrain", mesh);
	return mesh;
}


shared_ptr<Mesh> ResourceManager::LoadRectangleMesh()
{

	{
		shared_ptr<Mesh> findMesh = Get<Mesh>(L"UIQuad");
		if (findMesh)
			return findMesh;

		vector<Vertex> vertices(4);

		// 좌표: 0~1 UI 로컬 공간
		// UV와 1:1 매칭
		vertices[0] = Vertex{ Vec3(0.0f, 0.0f, 0), Vec2(0.0f, 0.0f) }; // LT
		vertices[1] = Vertex{ Vec3(1.0f, 0.0f, 0), Vec2(1.0f, 0.0f) }; // RT
		vertices[2] = Vertex{ Vec3(1.0f, 1.0f, 0), Vec2(1.0f, 1.0f) }; // RB
		vertices[3] = Vertex{ Vec3(0.0f, 1.0f, 0), Vec2(0.0f, 1.0f) }; // LB

		vector<uint32> indices(6);
		indices[0] = 0; indices[1] = 1; indices[2] = 2;
		indices[3] = 0; indices[4] = 2; indices[5] = 3;

		shared_ptr<Mesh> mesh = make_shared<Mesh>();
		mesh->Init(vertices, indices);   // UI 전용 Init 오버로드 권장
		Add(L"UIQuad", mesh);

		
	}



	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Rectangle");
	if (findMesh)
		return findMesh;

	float w2 = 0.5f;
	float h2 = 0.5f;

	vector<Vertex> vec(4);

	// �ո�
	vec[0] = Vertex(Vec3(-w2, -h2, 0), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-w2, +h2, 0), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+w2, +h2, 0), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+w2, -h2, 0), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));

	vector<uint32> idx(6);

	// �ո�
	idx[0] = 0; idx[1] = 1; idx[2] = 2;
	idx[3] = 0; idx[4] = 2; idx[5] = 3;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Rectangle", mesh);

	return mesh;
}

shared_ptr<Mesh> ResourceManager::LoadCubeMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Cube");
	if (findMesh)
		return findMesh;

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
	mesh->Init(vec, idx);
	Add(L"Cube", mesh);

	return mesh;
}

shared_ptr<Mesh> ResourceManager::LoadWireCubeMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"WireCube");
	if (findMesh)
		return findMesh;

	const float h = 0.5f;

	vector<Vertex> v(8);
	auto MakeV = [](float x, float y, float z)
		{
			Vertex vv{};
			vv.pos = Vec3(x, y, z);
			// uv/normal/tangent 등은 0이어도 됨(디버그 라인 셰이더에서 안 씀)
			return vv;
		};

	v[0] = MakeV(-h, -h, -h);
	v[1] = MakeV(+h, -h, -h);
	v[2] = MakeV(+h, +h, -h);
	v[3] = MakeV(-h, +h, -h);
	v[4] = MakeV(-h, -h, +h);
	v[5] = MakeV(+h, -h, +h);
	v[6] = MakeV(+h, +h, +h);
	v[7] = MakeV(-h, +h, +h);

	// 12 edges => 24 indices (LINELIST)
	vector<uint32> i =
	{
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(v, i);
	Add(L"WireCube", mesh);
	return mesh;
}

shared_ptr<Mesh> ResourceManager::LoadSphereMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Sphere");
	if (findMesh)
		return findMesh;

	float radius = 0.5f; // ���� ������
	uint32 stackCount = 20; // ���� ����
	uint32 sliceCount = 20; // ���� ����

	vector<Vertex> vec;

	Vertex v;

	// �ϱ�
	v.pos = Vec3(0.0f, radius, 0.0f);
	v.uv = Vec2(0.5f, 0.0f);
	v.normal = v.pos;
	v.normal.Normalize();
	v.tangent = Vec3(1.0f, 0.0f, 1.0f);
	vec.push_back(v);

	float stackAngle = XM_PI / stackCount;
	float sliceAngle = XM_2PI / sliceCount;

	float deltaU = 1.f / static_cast<float>(sliceCount);
	float deltaV = 1.f / static_cast<float>(stackCount);

	// ������� ���鼭 ������ ����Ѵ� (�ϱ�/���� �������� ����� X)
	for (uint32 y = 1; y <= stackCount - 1; ++y)
	{
		float phi = y * stackAngle;

		// ����� ��ġ�� ����
		for (uint32 x = 0; x <= sliceCount; ++x)
		{
			float theta = x * sliceAngle;

			v.pos.x = radius * sinf(phi) * cosf(theta);
			v.pos.y = radius * cosf(phi);
			v.pos.z = radius * sinf(phi) * sinf(theta);

			v.uv = Vec2(deltaU * x, deltaV * y);

			v.normal = v.pos;
			v.normal.Normalize();

			v.tangent.x = -radius * sinf(phi) * sinf(theta);
			v.tangent.y = 0.0f;
			v.tangent.z = radius * sinf(phi) * cosf(theta);
			v.tangent.Normalize();

			vec.push_back(v);
		}
	}

	// ����
	v.pos = Vec3(0.0f, -radius, 0.0f);
	v.uv = Vec2(0.5f, 1.0f);
	v.normal = v.pos;
	v.normal.Normalize();
	v.tangent = Vec3(1.0f, 0.0f, 0.0f);
	vec.push_back(v);

	vector<uint32> idx(36);

	// �ϱ� �ε���
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		//  [0]
		//   |  \
		//  [i+1]-[i+2]
		idx.push_back(0);
		idx.push_back(i + 2);
		idx.push_back(i + 1);
	}

	// ���� �ε���
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 y = 0; y < stackCount - 2; ++y)
	{
		for (uint32 x = 0; x < sliceCount; ++x)
		{
			//  [y, x]-[y, x+1]
			//  |		/
			//  [y+1, x]
			idx.push_back(1 + (y)*ringVertexCount + (x));
			idx.push_back(1 + (y)*ringVertexCount + (x + 1));
			idx.push_back(1 + (y + 1) * ringVertexCount + (x));
			//		 [y, x+1]
			//		 /	  |
			//  [y+1, x]-[y+1, x+1]
			idx.push_back(1 + (y + 1) * ringVertexCount + (x));
			idx.push_back(1 + (y)*ringVertexCount + (x + 1));
			idx.push_back(1 + (y + 1) * ringVertexCount + (x + 1));
		}
	}

	// ���� �ε���
	uint32 bottomIndex = static_cast<uint32>(vec.size()) - 1;
	uint32 lastRingStartIndex = bottomIndex - ringVertexCount;
	for (uint32 i = 0; i < sliceCount; ++i)
	{
		//  [last+i]-[last+i+1]
		//  |      /
		//  [bottom]
		idx.push_back(bottomIndex);
		idx.push_back(lastRingStartIndex + i);
		idx.push_back(lastRingStartIndex + i + 1);
	}

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Sphere", mesh);

	return mesh;
}

shared_ptr<FBXData> ResourceManager::LoadFBX(const wstring& path)
{
	shared_ptr<FBXData> meshData = Get<FBXData>(s2ws(filesystem::path(path).filename().stem().string()));
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->Load(path);
	meshData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(s2ws(filesystem::path(path).filename().stem().string()), meshData);

	return meshData;
}

shared_ptr<FBXData> ResourceManager::LoadFBXMesh(const wstring& path)
{
	shared_ptr<FBXData> meshData = Get<FBXData>(s2ws(filesystem::path(path).filename().stem().string()));
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->LoadMeshOnly(path);
	meshData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(s2ws(filesystem::path(path).filename().stem().string()), meshData);

	return meshData;
}

shared_ptr<Vfx> ResourceManager::LoadEffect(const wstring& path)
{
	shared_ptr<Vfx> effect = Get<Vfx>(s2ws(filesystem::path(path).filename().stem().string()));
	if(effect)
		return effect;
	effect = make_shared<Vfx>();
	effect ->Load(path);
	effect ->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(effect->GetName(), effect);

	return effect;
}

void ResourceManager::LoadAllTexture(const wstring& path)
{
	// std::string filePath{ filesystem::path(path).parent_path().string() + "\\" + filesystem::path(path).filename().stem().string() };

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


shared_ptr<Texture> ResourceManager::CreateTexture(const wstring& name, DXGI_FORMAT format, uint32 width, uint32 height,
	const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
	D3D12_RESOURCE_FLAGS resFlags, bool createSRVUAV, int msaaCount, int msaaQuilty, Vec4 clearColor, uint16 arraySize, TextureType type)
{
	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->Create(format, width, height, heapProperty, heapFlags, resFlags, createSRVUAV, msaaCount, msaaQuilty, clearColor, arraySize, type);
	Add(name, texture);

	return texture;
}

shared_ptr<Texture> ResourceManager::CreateTextureFromResource(const wstring& name, ComPtr<ID3D12Resource> tex2D, bool createSRVUAV)
{
	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->CreateFromResource(tex2D , createSRVUAV);
	Add(name, texture);

	return texture;
}

void ResourceManager::CreateDefaultRootSignature()
{
	// type count baseReg baseSpace

	// GraphicsRootSignature
	{

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges0 =	// g- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBUFFER_INDEX_COUNT, 0,0), // t1~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성

		};


		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges1 =	// group- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CONSTANT_INDEX_COUNT, 0,1), // b1~b4 몇번부터 몇개까지 레지스터를 사용할건지 작성
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  GROUP_SRV_COUNT, 0,1), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,  GROUP_UAV_COUNT, 0,1), // u0 사용

		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges2 =	// particle- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  1,0 ,2),
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2,0,2),
		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges3 =	// animation- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ANIMATION_INDEX_COUNT,0 ,3),
		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges4 =	// texture- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  1, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_MATERIALS_INDEX),4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  TEXTURE_CUBE_COUNT, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_CUBE_INDEX),4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  TEXTURE_SRV_COUNT, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_CUBE_INDEX) + TEXTURE_CUBE_COUNT,4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
		};

		shared_ptr<RootSignature> rootSignature = make_shared<RootSignature>();

		Add<RootSignature>(L"MainRootSignature", rootSignature);
		rootSignature->AddConstant(0, 3);
		rootSignature->AddTable(ranges0);
		rootSignature->AddTable(ranges1);
		rootSignature->AddTable(ranges2);
		rootSignature->AddTable(ranges3);
		rootSignature->AddTable(ranges4);
		//rootSignature->AddSampler(CD3DX12_STATIC_SAMPLER_DESC(0));

		CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0);
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		rootSignature->AddSampler(samplerDesc);

		CD3DX12_STATIC_SAMPLER_DESC samplerDesc2(1);
		samplerDesc2.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc2.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc2.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc2.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc2.MipLODBias = 0.0f;
		samplerDesc2.MaxAnisotropy = 16;
		samplerDesc2.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc2.MinLOD = 0.0f;
		samplerDesc2.MaxLOD = D3D12_FLOAT32_MAX;
		rootSignature->AddSampler(samplerDesc2);



		rootSignature->CreateGraphicsRootSignature();

	}

	// ComputeRootSignature
	//{
	//	std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges =
	//	{
	//		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  static_cast<uint8>(CONSTANT_INDEX::CBV_INDEX_END), 0), // b0~b4
	//		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  static_cast<uint8>(CONSTANT_INDEX::CBV_INDEX_END), 0), // t0~t9
	//		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,  static_cast<uint8>(CONSTANT_INDEX::CBV_INDEX_END), 0), // u0~u4
	//	};

	//	shared_ptr<RootSignature> rootSignature = make_shared<RootSignature>();

	//	Add<RootSignature>(L"ComputeRootSignature", rootSignature);
	//	RESOURCEMANAGER.Get<RootSignature>(L"ComputeRootSignature")->AddTable(ranges);
	//	RESOURCEMANAGER.Get<RootSignature>(L"ComputeRootSignature")->CreateComputeRootSignature();

	//}

}

void ResourceManager::CreateDefaultShader()
{
	

	// Skybox					현재 swapChain에 박고 있음
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_EQUAL

		};

		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\skybox_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\skybox_PS.hlsl"
		};

		shared_ptr<Shader> shader = make_shared<Shader>();

		shader->CreateGraphicsShader(shaderPath, info, 4, ShaderArg());

		Add<Shader>(L"Skybox", shader);
	}

	// Terrain Shadow
	{
		ShaderInfo info =
		{
			SHADER_TYPE::SHADOW,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS,
			BLEND_TYPE::DEFAULT,
			D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
		};

		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Terrain_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\TerrainShadow_PS.hlsl",
			.HS = L"..\\Resources\\Shader\\Terrain_HS.hlsl",
			.DS = L"..\\Resources\\Shader\\TerrainShadow_DS.hlsl",
		};

		ShaderArg arg =
		{
			"VS_Main",
			"HS_Main",
			"DS_Main",
			"",
			"PS_Main",
		};

		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, arg);
		Add<Shader>(L"TerrainShadow", shader);
	}

	// Terrain
	{
		ShaderInfo info =
		{
			SHADER_TYPE::DEFERRED,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS,
			BLEND_TYPE::DEFAULT,
			D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
		};

		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Terrain_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\Terrain_PS.hlsl",
			.HS = L"..\\Resources\\Shader\\Terrain_HS.hlsl",
			.DS = L"..\\Resources\\Shader\\Terrain_DS.hlsl",
		};

		ShaderArg arg =
		{
			"VS_Main",
			"HS_Main",
			"DS_Main",
			"",
			"PS_Main",
		};


		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1 ,arg);
		Add<Shader>(L"Terrain", shader);
	}

	//// Cel (Default -cel) - TO-DO
	//{
	//	ShaderInfo info =
	//	{
	//		SHADER_TYPE::DEFERRED
	//	};

	//	ShaderPath shaderPath{
	//	.VS = L"..\\Resources\\Shader\\cel_VS.hlsl",
	//	.PS = L"..\\Resources\\Shader\\cel_PS.hlsl"
	//	};


// Deferred (Deferred)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::DEFERRED,

		};

		ShaderPath shaderPath{
		.VS = L"..\\Resources\\Shader\\deferred_VS.hlsl",
		.PS = L"..\\Resources\\Shader\\deferred_PS.hlsl"
		};

		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,1, "VS_Main", "PS_Main");
		Add<Shader>(L"Deferred", shader);
	}


	// Forward (Forward)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\forward_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 4, ShaderArg());
		Add<Shader>(L"Forward", shader);
	}

	
	// Texture (Forward)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\texture_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\texture_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 4, "VS_Tex", "PS_Tex");
		Add<Shader>(L"Texture", shader);
	}

	// DirLight
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LIGHTING,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ONE_TO_ONE_BLEND
		};
		ShaderPath shaderPath{
		.VS = L"..\\Resources\\Shader\\lighting_dir_VS.hlsl",
		.PS = L"..\\Resources\\Shader\\lighting_dir_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,1, "VS_DirLight", "PS_DirLight");
		Add<Shader>(L"DirLight", shader);
	}

	// PointLight
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LIGHTING,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ONE_TO_ONE_BLEND
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\lighting_point_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\lighting_point_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,1, "VS_PointLight", "PS_PointLight");
		Add<Shader>(L"PointLight", shader);
	}

	// Final
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LIGHTING,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\final_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\final_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Final", "PS_Final");
		Add<Shader>(L"Final", shader);
	}

	// Compute Shader (프로젝트 제외함)
	//{
	//	ShaderPath shaderPath{
	//		.CS = L"..\\Resources\\Shader\\compute.hlsl",
	//	};
	//	shared_ptr<Shader> shader = make_shared<Shader>();
	//	shader->CreateComputeShader(shaderPath, "CS_Main");
	//	Add<Shader>(L"ComputeShader", shader);
	//}

	// Particle
	{
		ShaderInfo info =
		{
			SHADER_TYPE::PARTICLE,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
			D3D_PRIMITIVE_TOPOLOGY_POINTLIST
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\particle_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\particle_PS.hlsl",
			.GS = L"..\\Resources\\Shader\\particle_GS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,1, "VS_Main", "PS_Main", "GS_Main");
		Add<Shader>(L"Particle", shader);
	}

	// ComputeParticle
	{
	 	ShaderPath shaderPath{
			.CS = L"..\\Resources\\Shader\\particle_CS.hlsl",
		};
	 
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateComputeShader(shaderPath, "CS_Main");
		Add<Shader>(L"ComputeParticle", shader);
	}

	// Shadow
	{
		ShaderInfo info =
		{
			SHADER_TYPE::SHADOW,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\shadow_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\shadow_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"Shadow", shader);
	}


	// UI			현재 swapChain에 박고 있음
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\UI_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\UI_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,1, ShaderArg());
		Add<Shader>(L"UI", shader);
	}

	// animation 
	{

		ShaderPath shaderPath{
			.CS = L"..\\Resources\\Shader\\animation_CS.hlsl",
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateComputeShader(shaderPath);
		Add<Shader>(L"AnimationComputeShader", shader);
	
	}

	// DebugLine (Depth Test O, Depth Write X)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST // [핵심]
		};

		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\debugline_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\debugline_PS.hlsl"
		};

		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 4, "VS_Main", "PS_Main");
		Add<Shader>(L"DebugLine", shader);
	}

	// DebugLine (항상 보이게: Depth Test X)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST
		};

		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\debugline_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\debugline_PS.hlsl"
		};

		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info,4, "VS_Main", "PS_Main");
		Add<Shader>(L"DebugLine_NoDepth", shader);
	}
}

void ResourceManager::CreateDefaultMaterial()
{

	// Skybox
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Skybox");
		material->SetTexture(Load<Texture>(L"SkyboxTexture", L"..\\Resources\\Texture\\Hdri_Sky.dds"), DIFFUSEMAP0INDEX);
		Add<Material>(L"Skybox", material);
	}




	// 추후 주석된 부분은 GBUFFER전용 생성으로 폐기 예정임.
	 //DirLight
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"DirLight");
		//material->SetTexture(Get<Texture>(L"PositionTarget"), );
		//material->SetTexture(1, Get<Texture>(L"NormalTarget"));
		Add<Material>(L"DirLight", material);
	}

	// PointLight
	{
		const WindowInfo& window = RENDERMANAGER.GetWindow();
		Vec2 resolution = { static_cast<float>(window.Width), static_cast<float>(window.Height) };


		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"PointLight");
		//material->SetTexture(0, Get<Texture>(L"PositionTarget"));
		//material->SetTexture(1, Get<Texture>(L"NormalTarget"));
		//material->SetVec2(0, resolution);
		Add<Material>(L"PointLight", material);
	}

	// Final
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Final");
		//material->SetTexture(0, Get<Texture>(L"DiffuseTarget"));
		//material->SetTexture(1, Get<Texture>(L"DiffuseLightTarget"));
		//material->SetTexture(2, Get<Texture>(L"SpecularLightTarget"));
		Add<Material>(L"Final", material);
	}
	  
	// Terrain
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Terrain");
		//material->SetTexture(Load<Texture>(L"HeightMap0", L"..\\Resources\\Terrain\\Asphalt.png"), DIFFUSEMAP0INDEX);
		//material->SetTexture(Load<Texture>(L"HeightMap1", L"..\\Resources\\Terrain\\Base_Texture.jpg"), DIFFUSEMAP1INDEX);
		material->SetTexture(Load<Texture>(L"T_Height", L"..\\Resources\\Terrain\\T_Height.png"), DIFFUSEMAP2INDEX);
		Add<Material>(L"Terrain", material);
	}
	// Terrain 1
	{

		shared_ptr<Material> color1 = make_shared<Material>();
		color1->SetShader(L"Terrain");
		color1->SetTexture(Load<Texture>(L"T_Rock_BC", L"..\\Resources\\Terrain\\T_Rock_BC.png"), DIFFUSEMAP0INDEX);
		color1->SetTexture(Load<Texture>(L"T_Rock_Layer", L"..\\Resources\\Terrain\\T_Rock_Layer.png"), DIFFUSEMAP1INDEX);
		color1->SetTexture(Load<Texture>(L"colors", L"..\\Resources\\Terrain\\Geom_Rock_Overgrown_B_LOD00_Rock_Overgrown_B_0_Normal.png"), NORMALMAPINDEX);
		Add<Material>(L"Rock", color1);


		//
		shared_ptr<Material> color2 = make_shared<Material>();
		color2->SetShader(L"Terrain");
		color2->SetTexture(Load<Texture>(L"T_Dirt_BC", L"..\\Resources\\Terrain\\T_Dirt_BC.png"), DIFFUSEMAP0INDEX);
		color2->SetTexture(Load<Texture>(L"T_Dirt_Layer", L"..\\Resources\\Terrain\\T_Dirt_Layer.png"), DIFFUSEMAP1INDEX);
		color2->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		Add<Material>(L"Dirt", color2);

		//
		shared_ptr<Material> color3 = make_shared<Material>();
		color3->SetShader(L"Terrain");
		color3->SetTexture(Load<Texture>(L"T_Grass_BC", L"..\\Resources\\Terrain\\T_Grass_BC.dds"), DIFFUSEMAP0INDEX);
		color3->SetTexture(Load<Texture>(L"T_Grass_Layer", L"..\\Resources\\Terrain\\T_Grass_Layer.png"), DIFFUSEMAP1INDEX);
		color3->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		Add<Material>(L"Grass", color3);

		//shared_ptr<Material> color3 = make_shared<Material>();
		//color3->SetShader(L"Terrain");
		//color3->SetTexture(Load<Texture>(L"T_Ground_Gravel_BC", L"..\\Resources\\Terrain\\T_Ground_Gravel_BC.png"), DIFFUSEMAP0INDEX);
		//color3->SetTexture(Load<Texture>(L"Ground_Gravel", L"..\\Resources\\Terrain\\Ground_Gravel.png"), DIFFUSEMAP1INDEX);
		//color3->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		//Add<Material>(L"Ground_Gravel", color3);

		//shared_ptr<Material> color4 = make_shared<Material>();
		//color4->SetShader(L"Terrain");
		//color4->SetTexture(Load<Texture>(L"T_Mosaic_BC", L"..\\Resources\\Terrain\\T_Mosaic_BC.png"), DIFFUSEMAP0INDEX);
		//color4->SetTexture(Load<Texture>(L"Mosaic", L"..\\Resources\\Terrain\\Mosaic.png"), DIFFUSEMAP1INDEX);
		//color4->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		//Add<Material>(L"Mosaic", color4);

		//shared_ptr<Material> color5 = make_shared<Material>();
		//color5->SetShader(L"Terrain");
		//color5->SetTexture(Load<Texture>(L"T_SnowFootprints_BC", L"..\\Resources\\Terrain\\T_SnowFootprints_BC.png"), DIFFUSEMAP0INDEX);
		//color5->SetTexture(Load<Texture>(L"SnowFootprints", L"..\\Resources\\Terrain\\SnowFootprints.png"), DIFFUSEMAP1INDEX);
		//color5->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		//Add<Material>(L"SnowFootprints", color5);

		//shared_ptr<Material> color6 = make_shared<Material>();
		//color6->SetShader(L"Terrain");
		//color6->SetTexture(Load<Texture>(L"T_Soil_Mud", L"..\\Resources\\Terrain\\T_Soil_Mud.png"), DIFFUSEMAP0INDEX);
		//color6->SetTexture(Load<Texture>(L"Soil_Mud", L"..\\Resources\\Terrain\\Soil_Mud.png"), DIFFUSEMAP1INDEX);
		//color6->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
		//Add<Material>(L"Soil_Mud", color6);


	}	// Terrain
	
	//////
	//// Compute Shader (프로젝트 제외함)
	//{

	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"ComputeShader");
	//	Add<Material>(L"ComputeShader", material);
	//}
	////////

	// Particle
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Particle");
		Add<Material>(L"Particle", material);
	}

	// ComputeParticle
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"ComputeParticle");

		Add<Material>(L"ComputeParticle", material);
	}


	//HPBAR
	{
		shared_ptr<Texture> texture = Load<Texture>(L"HPBAR", L"..\\Resources\\Image\\UI\\UI_Hpbar_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"HPBAR", material);
	}

	//IbanixPortrait
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Ibanix_Portrait", L"..\\Resources\\Image\\UI\\UI_Ibanix_Portrait_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Ibanix_Portrait", material);
	}

	//FanthorPortrait
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Fanthor_Portrait", L"..\\Resources\\Image\\UI\\UI_Fanthor_Portrait_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Fanthor_Portrait", material);
	}


	//RudwigPortrait
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Rudwig_Portrait", L"..\\Resources\\Image\\UI\\UI_Rudwig_Portrait_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Rudwig_Portrait", material);
	}

	//Aim
	{
		shared_ptr<Texture> texture = Load<Texture>(L"jAims", L"..\\Resources\\Image\\UI\\UI_Aim_01.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"jAims", material);
	}

	//Ibanix_Ammo
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Ibanix_Ammo", L"..\\Resources\\Image\\UI\\UI_Ibanix_Ammo_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Ibanix_Ammo", material);
	}

	//Ibanix_Rhythm
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Ibanix_Rhythm", L"..\\Resources\\Image\\UI\\UI_Ibanix_Rhythm_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Ibanix_Rhythm", material);
	}

	//Ibanix_Skill_1
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Ibanix_Skill_01", L"..\\Resources\\Image\\UI\\UI_Ibanix_Skill_01.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Ibanix_Skill_01", material);
	}

	//Ibanix_Skill_2
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Ibanix_Skill_02", L"..\\Resources\\Image\\UI\\UI_Ibanix_Skill_02.dds");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"Ibanix_Skill_02", material);
	}


	// DebugLine Material
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"DebugLine");
		Add<Material>(L"DebugLine", material);
	}

	// DebugLine_NoDepth Material
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"DebugLine_NoDepth");
		Add<Material>(L"DebugLine_NoDepth", material);
	}

	// ResourceManager::CreateDefaultMaterial() 안에 추가
	{
		// DebugLine_Green
		auto mat = make_shared<Material>();
		mat->SetShader(L"DebugLine");                 // 동일 셰이더
		mat->GetParams().Diffuse = Vec4(0.f, 1.f, 0.f, 1.f);                           
		Add<Material>(L"DebugLine_Green", mat);
	}

	{
		// DebugLine_Red
		auto mat = make_shared<Material>();
		mat->SetShader(L"DebugLine");
		mat->GetParams().Diffuse = Vec4(1.f, 0.f, 0.f, 1.f);
		Add<Material>(L"DebugLine_Red", mat);
	}

	// GameObject
	//{

	//	shared_ptr<Texture> texture = Load<Texture>(L"Leather", L"..\\Resources\\Texture\\Leather.jpg");
	//	shared_ptr<Texture> texture2 = Load<Texture>(L"Leather_Normal", L"..\\Resources\\Texture\\Leather_Normal.jpg");
	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"Deferred");
	//	material->SetTexture(texture, DIFFUSEMAP0INDEX);
	//	material->SetTexture(texture2, NORMALMAPINDEX);
	//	Add<Material>(L"GameObject", material);
	//}


	//.fbx 라는 뜻은 진짜 fbx 파일을 로드한다는 뜻이 아니라
	// .ani, .mesh, .skel의 파일을 묶어서 fbx라는 이름으로 임의로 가져온다는 뜻임
	//따라서 진짜 fbx 파일을 로드하지 않아도 됨.

	LoadFBX(L"..\\Resources\\FBX\\oo1.fbx");
	//LoadFBX(L"..\\Resources\\FBX\\XYZ.fbx");
	//LoadFBX(L"..\\Resources\\FBX\\ZUP_Ascii_3dmax_Pivot.fbx");
	//LoadFBX(L"..\\Resources\\FBX\\YUP_Ascii_3dmax_Pivot.fbx");

	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Attack_01.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Idle.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Walk.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Jump.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Run.fbx");
	
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Land.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Fall.fbx");

	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Attack_01.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Idle.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Jump.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Run.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Walk.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Land.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Fall.fbx");


	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Attack01.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Idle.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Jump.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Run.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Walk.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Land.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Fall.fbx");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Noteboar\\SK_NoteBoar_Run.fbx");

	LoadEffect(L"..\\Resources\\Effect\\VFX_Noteboar_dissolve\\vfx_dissolve_NoteBoar.efk");
}