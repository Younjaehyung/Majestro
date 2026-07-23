#include "pch.h"
#include "ResourceManager.h"
#include "Engine.h"
#include "RenderManager.h"
#include "RootSignature.h"
#include "PayloadPathData.h"
#include "JsonUtils.h"
#include "GpuResourceBudget.h"
#include "EngineLog.h"



void ResourceManager::Initialize()
{
	CreateDefaultRootSignature();
	CreateDefaultShader();
	CreateDefaultMaterial();
	CreateDefaultParticleEffect();

	LoadPointMesh();
	LoadRectangleMesh();
	LoadPlaneMesh();
	LoadSphereMesh();
	LoadCubeMesh();

	LoadWireCubeMesh();
	LoadLineMesh();


	auto commandQueue = RENDERMANAGER.GetGraphicsCmdQueue();
	if (commandQueue && commandQueue->HasPendingStaticBufferUploads())
		commandQueue->FlushResourceCommandQueue();

	// 시작 시점 텍스처 예산 요약 출력
	DumpTextureBudget("startup");
	GpuResourceBudget::Dump("startup",
		RENDERMANAGER.GetDevice()->GetAdapter().Get(),
		RENDERMANAGER.GetGraphicsMemory().get());
}



shared_ptr<Mesh> ResourceManager::LoadLineMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Line");
	if (findMesh)
		return findMesh;

	// 단위 선분: (0,0,0)  (1,0,0)
	// 디버그 렌더러에서 월드 행렬로 임의의 두 점 사이의 선분으로 변환됨
	vector<Vertex> v(2);
	v[0].pos = Vec3(0.f, 0.f, 0.f);
	v[1].pos = Vec3(1.f, 0.f, 0.f);

	vector<uint32> i = { 0, 1 };

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(v, i);
	Add(L"Line", mesh);
	return mesh;
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

shared_ptr<NavMesh> ResourceManager::LoadNavMesh(const wstring& path)
{
	shared_ptr<NavMesh> findMesh = Get<NavMesh>(path);
	if (findMesh)
		return findMesh;
	findMesh = make_shared<NavMesh>();
	findMesh->Load(path);
	findMesh->SetName(L"NavMesh");
	if (findMesh->mDtNavMesh == nullptr) {
		std::cerr << "Failed to load NavMesh from path: " << ws2s(path) << "\n";
		return nullptr;
	}
	Add(L"NavMesh", findMesh);

	return findMesh;
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

	// UITriangle 
	{
		shared_ptr<Mesh> findTriMesh = Get<Mesh>(L"UITriangle");
		if (findTriMesh == nullptr)
		{
			vector<Vertex> triVertices(3);
			triVertices[0] = Vertex{ Vec3(0.0f, 0.0f, 0), Vec2(0.0f, 0.0f) }; // V0 (barycentric 1,0,0)
			triVertices[1] = Vertex{ Vec3(1.0f, 0.0f, 0), Vec2(1.0f, 0.0f) }; // V1 (barycentric 0,1,0)
			triVertices[2] = Vertex{ Vec3(1.0f, 1.0f, 0), Vec2(1.0f, 1.0f) }; // V2 (barycentric 0,0,1)

			vector<uint32> triIndices{ 0, 1, 2 };

			shared_ptr<Mesh> triMesh = make_shared<Mesh>();
			triMesh->Init(triVertices, triIndices);
			Add(L"UITriangle", triMesh);
		}
	}



	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Rectangle");
	if (findMesh)
		return findMesh;

	float w2 = 0.5f;
	float h2 = 0.5f;

	vector<Vertex> vec(4);

	//  ո 
	vec[0] = Vertex(Vec3(-w2, -h2, 0), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-w2, +h2, 0), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+w2, +h2, 0), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+w2, -h2, 0), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));

	vector<uint32> idx(6);

	//  ո 
	idx[0] = 0; idx[1] = 1; idx[2] = 2;
	idx[3] = 0; idx[4] = 2; idx[5] = 3;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Rectangle", mesh);

	return mesh;
}

shared_ptr<Mesh> ResourceManager::LoadPlaneMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"Plane");
	if (findMesh)
		return findMesh;

	// XZ 수평면, 법선 +Y, UV 0~1, 1x1
	float h = 0.5f;
	vector<Vertex> vec(4);
	vec[0] = Vertex(Vec3(-h, 0.f, -h), Vec2(0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-h, 0.f, +h), Vec2(0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+h, 0.f, +h), Vec2(1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+h, 0.f, -h), Vec2(1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));

	vector<uint32> idx{ 0, 1, 2, 0, 2, 3 };

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Plane", mesh);

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

	//  ո 
	vec[0] = Vertex(Vec3(-w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	//  ޸ 
	vec[4] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[5] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[6] = Vertex(Vec3(+w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[7] = Vertex(Vec3(-w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	//     
	vec[8] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[9] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[10] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[11] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	//  Ʒ   
	vec[12] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[13] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[14] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[15] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	//    ʸ 
	vec[16] = Vertex(Vec3(-w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[17] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[18] = Vertex(Vec3(-w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[19] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	//      ʸ 
	vec[20] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[21] = Vertex(Vec3(+w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[22] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[23] = Vertex(Vec3(+w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));

	vector<uint32> idx(36);

	//  ո 
	idx[0] = 0; idx[1] = 1; idx[2] = 2;
	idx[3] = 0; idx[4] = 2; idx[5] = 3;
	//  ޸ 
	idx[6] = 4; idx[7] = 5; idx[8] = 6;
	idx[9] = 4; idx[10] = 6; idx[11] = 7;
	//     
	idx[12] = 8; idx[13] = 9; idx[14] = 10;
	idx[15] = 8; idx[16] = 10; idx[17] = 11;
	//  Ʒ   
	idx[18] = 12; idx[19] = 13; idx[20] = 14;
	idx[21] = 12; idx[22] = 14; idx[23] = 15;
	//    ʸ 
	idx[24] = 16; idx[25] = 17; idx[26] = 18;
	idx[27] = 16; idx[28] = 18; idx[29] = 19;
	//      ʸ 
	idx[30] = 20; idx[31] = 21; idx[32] = 22;
	idx[33] = 20; idx[34] = 22; idx[35] = 23;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"Cube", mesh);

	return mesh;
}
shared_ptr<Mesh> ResourceManager::LoadMCubeMesh()
{
	shared_ptr<Mesh> findMesh = Get<Mesh>(L"MCube");
	if (findMesh)
		return findMesh;

	float w2 = 50.f;
	float h2 = 50.f;
	float d2 = 50.f;

	vector<Vertex> vec(24);

	//  ո 
	vec[0] = Vertex(Vec3(-w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[1] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[2] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[3] = Vertex(Vec3(+w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 0.0f));
	//  ޸ 
	vec[4] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[5] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[6] = Vertex(Vec3(+w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[7] = Vertex(Vec3(-w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f));
	//     
	vec[8] = Vertex(Vec3(-w2, +h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[9] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[10] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	vec[11] = Vertex(Vec3(+w2, +h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
	//  Ʒ   
	vec[12] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[13] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[14] = Vertex(Vec3(+w2, -h2, +d2), Vec2(0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	vec[15] = Vertex(Vec3(-w2, -h2, +d2), Vec2(1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f));
	//    ʸ 
	vec[16] = Vertex(Vec3(-w2, -h2, +d2), Vec2(0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[17] = Vertex(Vec3(-w2, +h2, +d2), Vec2(0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[18] = Vertex(Vec3(-w2, +h2, -d2), Vec2(1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	vec[19] = Vertex(Vec3(-w2, -h2, -d2), Vec2(1.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
	//      ʸ 
	vec[20] = Vertex(Vec3(+w2, -h2, -d2), Vec2(0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[21] = Vertex(Vec3(+w2, +h2, -d2), Vec2(0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[22] = Vertex(Vec3(+w2, +h2, +d2), Vec2(1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	vec[23] = Vertex(Vec3(+w2, -h2, +d2), Vec2(1.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));

	vector<uint32> idx(36);

	//  ո 
	idx[0] = 0; idx[1] = 1; idx[2] = 2;
	idx[3] = 0; idx[4] = 2; idx[5] = 3;
	//  ޸ 
	idx[6] = 4; idx[7] = 5; idx[8] = 6;
	idx[9] = 4; idx[10] = 6; idx[11] = 7;
	//     
	idx[12] = 8; idx[13] = 9; idx[14] = 10;
	idx[15] = 8; idx[16] = 10; idx[17] = 11;
	//  Ʒ   
	idx[18] = 12; idx[19] = 13; idx[20] = 14;
	idx[21] = 12; idx[22] = 14; idx[23] = 15;
	//    ʸ 
	idx[24] = 16; idx[25] = 17; idx[26] = 18;
	idx[27] = 16; idx[28] = 18; idx[29] = 19;
	//      ʸ 
	idx[30] = 20; idx[31] = 21; idx[32] = 22;
	idx[33] = 20; idx[34] = 22; idx[35] = 23;

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->Init(vec, idx);
	Add(L"MCube", mesh);

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

	float radius = 0.5f; //            
	uint32 stackCount = 20; //          
	uint32 sliceCount = 20; //          

	vector<Vertex> vec;

	Vertex v;

	//  ϱ 
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

	//            鼭            Ѵ  ( ϱ /                    X)
	for (uint32 y = 1; y <= stackCount - 1; ++y)
	{
		float phi = y * stackAngle;

		//         ġ       
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

	//     
	v.pos = Vec3(0.0f, -radius, 0.0f);
	v.uv = Vec2(0.5f, 1.0f);
	v.normal = v.pos;
	v.normal.Normalize();
	v.tangent = Vec3(1.0f, 0.0f, 0.0f);
	vec.push_back(v);

	vector<uint32> idx(36);

	//  ϱ   ε   
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		//  [0]
		//   |  \
		//  [i+1]-[i+2]
		idx.push_back(0);
		idx.push_back(i + 2);
		idx.push_back(i + 1);
	}

	//       ε   
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

	//       ε   
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

shared_ptr<FBXData> ResourceManager::LoadFBX(const wstring& path, const wstring& shader, const wstring& prefix)
{
	wstring stem = s2ws(filesystem::path(path).filename().stem().string());
	wstring key = MakeKey(prefix, stem);

	shared_ptr<FBXData> meshData = Get<FBXData>(key);
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->SetNamespace(prefix);          // 내부 mesh/material/texture 키에 prefix 전파
	meshData->Load(path, shader);
	meshData->SetName(key);
	Add(key, meshData);

	return meshData;
}

shared_ptr<FBXData> ResourceManager::LoadFBXMesh(const wstring& path, const wstring& prefix)
{
	wstring stem = s2ws(filesystem::path(path).filename().stem().string());
	wstring key = MakeKey(prefix, stem);

	shared_ptr<FBXData> meshData = Get<FBXData>(key);
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->SetNamespace(prefix);          // 내부 mesh/material/texture 키에 prefix 전파
	meshData->LoadMeshOnly(path);
	meshData->SetName(key);
	Add(key, meshData);

	return meshData;
}

shared_ptr<FBXData> ResourceManager::LoadFBXModel(const wstring& path, const wstring& prefix)
{
	wstring stem = s2ws(filesystem::path(path).filename().stem().string());
	wstring key = MakeKey(prefix, stem);

	shared_ptr<FBXData> meshData = Get<FBXData>(key);
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->SetNamespace(prefix);          // 내부 mesh/material/texture 키에 prefix 전파
	meshData->LoadModelOnly(path);
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
		if (EngineLog::Enabled(EngineLog::Domain::ResourceLoad))
		{
			EngineLog::Prefix(EngineLog::Domain::ResourceLoad, "collision")
				<< "key=" << ws2s(key)
				<< " old=" << ws2s(it->second)
				<< " new=" << ws2s(path) << "\n";
		}
#endif
}

namespace
{
	// Texture::Load와 동일한 디스크 경로 규칙(.dds 시블링 우선) — 해시는 실제로 읽힐 파일 기준이어야 함
	wstring ResolveTextureDiskPath(const wstring& path)
	{
		wstring ext = std::filesystem::path(path).extension();
		if (ext != L".dds" && ext != L".DDS")
		{
			wstring ddsPath = std::filesystem::path(path).replace_extension(L".dds").wstring();
			if (std::filesystem::exists(ddsPath))
				return ddsPath;
		}
		return path;
	}

	// FNV-1a 64bit — 파일 전체 바이트의 내용 지문. 읽기 실패 시 0 반환(호출부가 dedup 건너뜀)
	uint64 HashFileContent(const wstring& path)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		if (!file)
			return 0;

		uint64 hash = 14695981039346656037ull;
		char buffer[65536];
		while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
		{
			const std::streamsize n = file.gcount();
			for (std::streamsize i = 0; i < n; ++i)
			{
				hash ^= static_cast<uint8>(buffer[i]);
				hash *= 1099511628211ull;
			}
		}
		return hash == 0 ? 1 : hash; // 0은 실패 코드로 예약
	}
}

shared_ptr<Texture> ResourceManager::LoadTextureDeduped(const wstring& key, const wstring& path)
{
	OBJECT_TYPE objectType = GetObjectType<Texture>();
	KeyObjMap& keyObjMap = mResources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
	{
		DebugCheckKeyCollision(static_cast<uint8>(objectType), key, path);
		return static_pointer_cast<Texture>(findIt->second);
	}

	const uint64 contentHash = HashFileContent(ResolveTextureDiskPath(path));

	if (contentHash != 0)
	{
		auto cacheIt = mTextureContentCache.find(contentHash);
		if (cacheIt != mTextureContentCache.end())
		{
			// 내용 동일
			keyObjMap[key] = cacheIt->second;
			mTextureDedupCount++;
			mTextureBytesSaved += cacheIt->second->GetOriginalImage().GetPixelsSize();
			return cacheIt->second;
		}
	}

	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->Load(path);
	texture->SetName(key);
	keyObjMap[key] = texture;

	if (contentHash != 0)
		mTextureContentCache.emplace(contentHash, texture);

	const uint64 bytes = texture->GetOriginalImage().GetPixelsSize();
	mTextureBytesTotal += bytes;
	mTextureLoadLog.emplace_back(key, bytes);

	return texture;
}

shared_ptr<Texture> ResourceManager::ReloadTexture(const wstring& key, const wstring& path)
{
	OBJECT_TYPE objectType = GetObjectType<Texture>();
	KeyObjMap& keyObjMap = mResources[static_cast<uint8>(objectType)];
	shared_ptr<Texture> previousTexture;

	auto previous = keyObjMap.find(key);
	if (previous != keyObjMap.end())
	{
		previousTexture = static_pointer_cast<Texture>(previous->second);
		keyObjMap.erase(previous);
	}

	shared_ptr<Texture> reloadedTexture = LoadTextureDeduped(key, path);

	// 이전 텍스처가 없거나, 새로 로드된 텍스처가 이전 텍스처와 동일한 경우 캐시에서 제거할 필요 없음
	if (!previousTexture || reloadedTexture == previousTexture)
		return reloadedTexture;

	bool previousTextureIsStillUsed = false;

	// 이전 텍스처가 아직도 다른 키에서 사용 중인지 확인
	for (const auto& [resourceKey, resource] : keyObjMap)
	{
		if (resource.get() == previousTexture.get())
		{
			previousTextureIsStillUsed = true;
			break;
		}
	}
	
	// 이전 텍스처가 더 이상 사용되지 않는다면 캐시에서 제거
	if (!previousTextureIsStillUsed)
	{

		for (auto cache = mTextureContentCache.begin(); cache != mTextureContentCache.end();)
		{
			if (cache->second.get() == previousTexture.get())
				cache = mTextureContentCache.erase(cache);
			else
				++cache;
		}
	}
	

	return reloadedTexture;
}



void ResourceManager::DumpTextureBudget(const char* label)
{
	if (!EngineLog::Enabled(EngineLog::Domain::TextureBudget))
		return;

	auto sorted = mTextureLoadLog;
	std::sort(sorted.begin(), sorted.end(),
		[](const std::pair<wstring, uint64>& a, const std::pair<wstring, uint64>& b) { return a.second > b.second; });

	const double mb = 1024.0 * 1024.0;
	EngineLog::Prefix(EngineLog::Domain::TextureBudget, label)
		<< "loaded=" << sorted.size()
		<< " total=" << static_cast<uint32>(mTextureBytesTotal / mb) << "MB"
		<< " | dedup=" << mTextureDedupCount
		<< " saved=" << static_cast<uint32>(mTextureBytesSaved / mb) << "MB\n";

	const size_t topN = (std::min)(static_cast<size_t>(20), sorted.size());
	for (size_t i = 0; i < topN; ++i)
	{
		EngineLog::Prefix(EngineLog::Domain::TextureBudget, label)
			<< "resource=" << ws2s(sorted[i].first)
			<< " size=" << static_cast<uint32>(sorted[i].second / mb)
			<< "MB\n";
	}
}

void ResourceManager::UnloadSceneResources(const std::vector<wstring>& prefixes)
{
	if (prefixes.empty())
		return;

	auto isSceneKey = [&prefixes](const wstring& key)
	{
		for (const wstring& prefix : prefixes)
		{
			if (prefix.empty())
				continue;

			const wstring marker = prefix + L"/";
			if (key.rfind(marker, 0) == 0)
				return true;
		}
		return false;
	};

	uint32 removedObjects = 0;
	const OBJECT_TYPE sceneTypes[] =
	{
		OBJECT_TYPE::FBXDATA,
		OBJECT_TYPE::MESH,
		OBJECT_TYPE::MATERIAL,
		OBJECT_TYPE::TEXTURE,
		OBJECT_TYPE::ANIMATION,
		OBJECT_TYPE::SKELETON,
		OBJECT_TYPE::COLLIDER,
		OBJECT_TYPE::NAVMESH,
	};

	for (OBJECT_TYPE type : sceneTypes)
	{
		KeyObjMap& objects = mResources[static_cast<uint8>(type)];
		for (auto it = objects.begin(); it != objects.end();)
		{
			if (isSceneKey(it->first))
			{
				it = objects.erase(it);
				removedObjects++;
			}
			else
			{
				++it;
			}
		}
	}

	KeyObjMap& textures = mResources[static_cast<uint8>(OBJECT_TYPE::TEXTURE)];
	std::unordered_set<Texture*> liveTextures;
	for (const auto& [key, object] : textures)
	{
		if (object)
			liveTextures.insert(static_cast<Texture*>(object.get()));
	}

	for (auto it = mTextureContentCache.begin(); it != mTextureContentCache.end();)
	{
		Texture* texture = it->second.get();
		if (texture == nullptr || liveTextures.find(texture) == liveTextures.end())
			it = mTextureContentCache.erase(it);
		else
			++it;
	}

	uint64 removedTextureBytes = 0;
	for (auto it = mTextureLoadLog.begin(); it != mTextureLoadLog.end();)
	{
		if (isSceneKey(it->first))
		{
			removedTextureBytes += it->second;
			it = mTextureLoadLog.erase(it);
		}
		else
		{
			++it;
		}
	}

	mTextureBytesTotal = (removedTextureBytes > mTextureBytesTotal)
		? 0
		: (mTextureBytesTotal - removedTextureBytes);

	if (EngineLog::Enabled(EngineLog::Domain::SceneResource))
	{
		const double mb = 1024.0 * 1024.0;
		EngineLog::Prefix(EngineLog::Domain::SceneResource, "unload")
			<< "prefixes=";
		for (const wstring& prefix : prefixes)
			std::cout << ws2s(prefix) << " ";
		std::cout << "removedObjects=" << removedObjects
			<< " removedTextureMB=" << static_cast<uint32>(removedTextureBytes / mb)
			<< "\n";
	}
}

shared_ptr<Vfx> ResourceManager::LoadEffect(const wstring& path)
{
	shared_ptr<Vfx> effect = Get<Vfx>(s2ws(filesystem::path(path).filename().stem().string()));
	if (effect)
		return effect;
	effect = make_shared<Vfx>();
	effect->Load(path);
	effect->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(effect->GetName(), effect);

	return effect;
}

void ResourceManager::LoadAllTexture(const wstring& path)
{
	// std::string filePath{ filesystem::path(path).parent_path().string() + "\\" + filesystem::path(path).filename().stem().string() };

}

LevelImportData ResourceManager::LoadMapResourceJson(const std::wstring& path)
{
	std::string jsonPath = ws2s(path);
	std::ifstream ifs(jsonPath);
	if (!ifs.is_open())
		throw std::runtime_error("Failed to open json: " + jsonPath);

	json root;
	ifs >> root;

	LevelImportData out{};
	out.levelName = GetString(root, "level_name");

	// 맵별 스카이박스/IBL —{ "skybox", "irradiance", "prefiltered" }
	if (root.contains("sky") && root["sky"].is_object())
	{
		const auto& sky = root["sky"];
		out.sky.skybox      = GetOptionalString(sky, "skybox");
		out.sky.irradiance  = GetOptionalString(sky, "irradiance");
		out.sky.prefiltered = GetOptionalString(sky, "prefiltered");
		out.sky.valid = !out.sky.skybox.empty();
	}

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

			positionUnitScale = 100.0f;
		}
	}

	const auto& actors = RequireJson(root, "actors");
	if (!actors.is_array())
		throw std::runtime_error("JSON 'actors' is not an array");

	for (const auto& a : actors)
	{
		const std::string actorName = GetString(a, "name");
		const std::string actorPath = GetString(a, "path");

		const auto& comps = RequireJson(a, "static_mesh_components");
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

					const auto& dx = RequireJson(inst_j, "dx");

					insts.world = ParseDxTransform(dx);
					// insts.world.scale = insts.world.scale * positionUnitScale; // UE에서 cm 단위로 export 했을 때, DX에서 m 단위로 사용하려면 100배 해줘야 함
					//const auto& ue = RequireJson(inst_j, "ue");
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

				const auto& cwt = RequireJson(c, "component_world_transform");
				const auto& dx = RequireJson(cwt, "dx");
				inst.world = ParseDxTransform(dx);
				//const auto& ue = RequireJson(cwt, "ue");
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

void ResourceManager::ApplyMapSky(const LevelImportData& level)
{
	// sky 블록이 없으면 기본 스카이박스를 유지
	if (!level.sky.valid)
		return;

	const std::string lv = level.levelName;

	// <LevelName> 하위 우선
	auto resolve = [&](const std::string& stem) -> std::string {
		if (stem.empty())
			return "";
		if (!lv.empty())
		{
			std::string sub = "..\\Resources\\Texture\\" + lv + "\\" + stem + ".dds";
			if (std::filesystem::exists(sub))
				return sub;
		}
		return "..\\Resources\\Texture\\" + stem + ".dds";
	};

	// 파일별 캐시 키
	auto loadTex = [&](const std::string& stem) -> shared_ptr<Texture> {
		std::string path = resolve(stem);
		if (path.empty() || !std::filesystem::exists(path))
			return nullptr;
		return Load<Texture>(s2ws(stem), s2ws(path));
	};

	shared_ptr<Texture> skyboxTex = loadTex(level.sky.skybox);
	shared_ptr<Texture> iemTex    = loadTex(level.sky.irradiance);
	shared_ptr<Texture> pmremTex  = loadTex(level.sky.prefiltered);

	// Skybox / IBL 맵 텍스처로 교체 
	if (skyboxTex)
	{
		if (auto skyboxMat = Get<Material>(L"Skybox"))
			skyboxMat->SetTexture(skyboxTex, DIFFUSEMAP0INDEX);
		Replace<Texture>(L"SkyboxTexture", skyboxTex);
	}
	if (iemTex)
		Replace<Texture>(L"IBL_Irradiance", iemTex);
	if (pmremTex)
		Replace<Texture>(L"IBL_PreFiltered", pmremTex);
	// BRDF LUT(IBL_BrdfLut) 은 환경 무관이라 전역 유지 — 교체하지 않는다.
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


std::vector<Cinematic::CameraView> ResourceManager::LoadCameraViews(const std::wstring& path)
{
	using namespace Cinematic;
	std::vector<CameraView> views;

	std::ifstream ifs(ws2s(path));
	if (!ifs.is_open())
	{
		std::cout << "[LoadCameraViews] open failed: " << ws2s(path) << std::endl;
		return views;
	}

	json root;
	try { ifs >> root; }
	catch (const std::exception& e)
	{
		std::cout << "[LoadCameraViews] parse error: " << e.what() << std::endl;
		return views;
	}

	if (!root.contains("cameras") || !root["cameras"].is_array() || root["cameras"].empty())
		return views;

	const auto& cam0 = root["cameras"][0];
	if (!cam0.contains("path") || !cam0["path"].contains("samples"))
		return views;

	const auto& samples = cam0["path"]["samples"];
	if (!samples.is_array())
		return views;

	// 변환, 중복 판정은 압축
	CameraView staging{};
	for (const auto& s : samples)
	{
		ConvertMainMenuSample(s, staging);
		if (!views.empty() && IsSameMainMenuStop(views.back(), staging))
			continue;
		views.push_back(staging);
	}

	std::cout << "[LoadCameraViews] loaded " << views.size() << " views" << std::endl;
	for (size_t i = 0; i < views.size(); ++i)
	{
		const auto& v = views[i];
		std::cout << "  view[" << i << "] pos=(" << v.position.x << ", "
			<< v.position.y << ", " << v.position.z << ") fov=" << v.fovDeg << std::endl;
	}
	return views;
}

std::vector<Cinematic::CameraKeyframe> ResourceManager::LoadCameraSequence(const std::wstring& path)
{
	using namespace Cinematic;
	std::vector<CameraKeyframe> keys;

	std::ifstream ifs(ws2s(path));
	if (!ifs.is_open())
	{
		// 골격 단계: JSON 미준비 시 빈 시퀀스 반환(재생 안 함) — 정상 동작.
		std::cout << "[LoadCameraSequence] open failed: " << ws2s(path) << std::endl;
		return keys;
	}

	json root;
	try { ifs >> root; }
	catch (const std::exception& e)
	{
		std::cout << "[LoadCameraSequence] parse error: " << e.what() << std::endl;
		return keys;
	}

	if (!root.contains("cameras") || !root["cameras"].is_array() || root["cameras"].empty())
		return keys;

	const auto& cam0 = root["cameras"][0];
	if (!cam0.contains("path") || !cam0["path"].contains("samples"))
		return keys;

	const auto& samples = cam0["path"]["samples"];
	if (!samples.is_array())
		return keys;

	// LoadCameraViews와 달리 dedup하지 않고 전체 패스를 seconds와 함께 보존한다.
	keys.reserve(samples.size());
	for (const auto& s : samples)
	{
		CameraKeyframe kf{};
		ConvertMainMenuSample(s, kf.view);                 // position/basis/fov 변환 재사용
		kf.seconds = s.contains("seconds") ? GetFloat(s, "seconds") : 0.f;
		keys.push_back(kf);
	}

	std::cout << "[LoadCameraSequence] loaded " << keys.size() << " keyframes, duration="
		<< (keys.empty() ? 0.f : keys.back().seconds) << "s" << std::endl;
	return keys;
}


shared_ptr<Texture> ResourceManager::CreateTexture(const wstring& name, DXGI_FORMAT format, uint32 width, uint32 height,
	const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
	D3D12_RESOURCE_FLAGS resFlags, bool createSRVUAV, int msaaCount, int msaaQuilty, Vec4 clearColor, uint16 arraySize, TextureType type)
{
	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->Create(format, width, height, heapProperty, heapFlags, resFlags, createSRVUAV, msaaCount, msaaQuilty, clearColor, arraySize, type);

	Replace(name, texture);

	// GPU 리소스 예산 추적
	std::wstring group = L"CreatedTexture";

	if (resFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		group = L"DepthStencilTexture";
	else if (resFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		group = L"RenderTargetTexture";
	else if (resFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		group = L"UavTexture";

	GpuResourceBudget::RecordResource(DEVICE.Get(), group, name,
		heapProperty.Type, texture->GetTex2D().Get());

	return texture;
}

shared_ptr<Texture> ResourceManager::CreateTextureFromResource(const wstring& name, ComPtr<ID3D12Resource> tex2D, bool createSRVUAV)
{
	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->CreateFromResource(tex2D, createSRVUAV);

	Replace(name, texture);

	// GPU 리소스 예산 추적 추가
	// 스왑체인 백버퍼처럼 외부에서 받은 리소스도 예산표에 포함한다
	GpuResourceBudget::RecordResource(DEVICE.Get(), L"ExternalTextureResource",
		name, D3D12_HEAP_TYPE_DEFAULT, texture->GetTex2D().Get());

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
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,0,2),
		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges3 =	// particle spawn buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,1,2),
		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges4 =	// animation- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ANIMATION_INDEX_COUNT,0 ,3),
		};

		std::vector<CD3DX12_DESCRIPTOR_RANGE>  ranges5 =	// texture- buffer
		{
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  1, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_MATERIALS_INDEX),4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  TEXTURE_CUBE_COUNT, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_CUBE_INDEX),4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  TEXTURE_SRV_COUNT, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_CUBE_INDEX) + TEXTURE_CUBE_COUNT,4), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
		};

		shared_ptr<RootSignature> rootSignature = make_shared<RootSignature>();

		Add<RootSignature>(L"MainRootSignature", rootSignature);
		rootSignature->AddConstant(0, 16);
		rootSignature->AddTable(ranges0);
		rootSignature->AddTable(ranges1);
		rootSignature->AddTable(ranges2);
		rootSignature->AddTable(ranges3);
		rootSignature->AddTable(ranges4);
		rootSignature->AddTable(ranges5);
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

		// s2: 그림자 PCF 비교 샘플러 (SamplerComparisonState)
		CD3DX12_STATIC_SAMPLER_DESC samplerDescShadow(2);
		samplerDescShadow.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDescShadow.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDescShadow.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDescShadow.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDescShadow.MipLODBias = 0.0f;
		samplerDescShadow.MaxAnisotropy = 1;
		samplerDescShadow.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
		samplerDescShadow.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		samplerDescShadow.MinLOD = 0.0f;
		samplerDescShadow.MaxLOD = D3D12_FLOAT32_MAX;

		rootSignature->AddSampler(samplerDescShadow);

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
		shader->CreateGraphicsShader(shaderPath, info, 1, arg);
		Add<Shader>(L"Terrain", shader);
	}


// Deferred (Deferred)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::DEFERRED,
			RASTERIZER_TYPE::CULL_NONE,
			// LESS_EQUAL(Write ON): GBuffer가 자체 깊이를 생성하도록 변경.
			// DepthPrePass가 FORWARD-only로 축소되어도 DEFERRED 깊이가 GBuffer에서 채워짐.
			// (이전 EQUAL_NO_WRITE는 DepthPrePass 깊이에 완전 의존했음)
			DEPTH_STENCIL_TYPE::LESS_EQUAL
		};

		ShaderPath shaderPath{
		.VS = L"..\\Resources\\Shader\\deferred_VS.hlsl",
		.PS = L"..\\Resources\\Shader\\deferred_PS.hlsl"
		};

		shared_ptr<Shader> shader = make_shared<Shader>();

		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
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
		//shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 4, ShaderArg());
		Add<Shader>(L"Forward", shader);
	}


	// Ocean (Forward, 불투명)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_EQUAL,
			BLEND_TYPE::DEFAULT,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\ocean_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\ocean_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 4, ShaderArg());
		Add<Shader>(L"Ocean", shader);
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
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,  // 풀스크린 쿼드 — 깊이 테스트 불필요
			BLEND_TYPE::ONE_TO_ONE_BLEND
		};
		ShaderPath shaderPath{
		.VS = L"..\\Resources\\Shader\\lighting_dir_VS.hlsl",
		.PS = L"..\\Resources\\Shader\\lighting_dir_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		//shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_DirLight", "PS_DirLight");
		Add<Shader>(L"DirLight", shader);
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
		shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info,/* RENDERMANAGER.GetMsaaSampleCount()*/1, "VS_Final", "PS_Final");
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
		shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main", "GS_Main");
		Add<Shader>(L"Particle", shader);
	}

	// ParticlePSInstancing
	{
		ShaderInfo info =
		{
			SHADER_TYPE::PARTICLE,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\particle_instanced_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\particle_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"ParticleInstanced", shader);
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

	// SmokeRise 
	{
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
				.PS = L"..\\Resources\\Shader\\particle_smoke_PS.hlsl",
				.GS = L"..\\Resources\\Shader\\particle_GS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main", "GS_Main");
			Add<Shader>(L"SmokeParticle", shader);
		}

		{
			ShaderPath shaderPath{
			.CS = L"..\\Resources\\Shader\\particle_smoke_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeSmokeParticle", shader);
		}



		// SmokePsInstancing
		{
			{
				ShaderInfo info =
				{
					SHADER_TYPE::PARTICLE,
					RASTERIZER_TYPE::CULL_BACK,
					DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
					BLEND_TYPE::ALPHA_BLEND,
					D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
				};
				ShaderPath shaderPath{
					.VS = L"..\\Resources\\Shader\\particle_instanced_VS.hlsl",
					.PS = L"..\\Resources\\Shader\\particle_smoke_PS.hlsl"
				};
				shared_ptr<Shader> shader = make_shared<Shader>();
				shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
				shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
				Add<Shader>(L"SmokeParticleInstanced", shader);
			}
		}
	}

	// PlazaCloudDrift
	{
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ALPHA_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_cloud_drift_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_cloud_drift_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"CloudParticleInstanced", shader);
		}

		{
			ShaderPath shaderPath{
				.CS = L"..\\Resources\\Shader\\particle_cloud_drift_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeCloudParticle", shader);
		}

		{
			ShaderPath shaderPath{
				.CS = L"..\\Resources\\Shader\\particle_cloud_drift_near_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeCloudParticleNear", shader);
		}
	}

	// AuraRise
	{
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_POINTLIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_aura_PS.hlsl",
				.GS = L"..\\Resources\\Shader\\particle_GS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main", "GS_Main");
			Add<Shader>(L"AuraParticle", shader);
		}

		{
			ShaderPath shaderPath{
				.CS = L"..\\Resources\\Shader\\particle_aura_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeAuraParticle", shader);
		}

		// AuraPsInstancing
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_instanced_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_aura_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"AuraParticleInstanced", shader);
		}
	}


	// OrbitParticle
	{
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_POINTLIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_orbit_PS.hlsl",
				.GS = L"..\\Resources\\Shader\\particle_GS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main", "GS_Main");
			Add<Shader>(L"OrbitParticle", shader);
		}

		{
			ShaderPath shaderPath{
			.CS = L"..\\Resources\\Shader\\particle_orbit_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeOrbitParticle", shader);
		}


		// OrbitPsInstancing
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_instanced_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_orbit_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"OrbitParticleInstanced", shader);
		}
	}


	// BulletTrail
	{
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_POINTLIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_bullet_trail_PS.hlsl",
				.GS = L"..\\Resources\\Shader\\particle_GS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main", "GS_Main");
			Add<Shader>(L"BulletTrailParticle", shader);
		}

		{
			ShaderPath shaderPath{
				.CS = L"..\\Resources\\Shader\\particle_bullet_trail_CS.hlsl",
			};

			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateComputeShader(shaderPath, "CS_Main");
			Add<Shader>(L"ComputeBulletTrailParticle", shader);
		}

		// BulletTrailPsInstancing
		{
			ShaderInfo info =
			{
				SHADER_TYPE::PARTICLE,
				RASTERIZER_TYPE::CULL_BACK,
				DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
				BLEND_TYPE::ONE_TO_ONE_BLEND,
				D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			};
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\particle_instanced_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\particle_bullet_trail_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"BulletTrailParticleInstanced", shader);
		}
	}


	// WeaponTrail
	{
		ShaderInfo info =
		{
			SHADER_TYPE::PARTICLE,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
			BLEND_TYPE::ONE_TO_ONE_BLEND,
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\trail_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\trail_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"WeaponTrail", shader);
	}

	// Shadow
	{
		ShaderInfo info =
		{
			SHADER_TYPE::SHADOW,
			RASTERIZER_TYPE::CULL_NONE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\shadow_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\shadow_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"Shadow", shader);
	}

	// Shadow (알파 컷아웃 식생 전용 — 텍스처 알파 clip 후 depth 기록)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::SHADOW,
			RASTERIZER_TYPE::CULL_NONE,  // 풀잎 양면
			DEPTH_STENCIL_TYPE::LESS,
			BLEND_TYPE::ALPHA_TEST,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\shadow_alpha_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\shadow_alpha_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"ShadowAlpha", shader);
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
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"UI", shader);
	}

	// AudioVisualizer — visualizer_VS(BaseInstanceID 오프셋) + visualizer_PS
	// UIInstanceData의 MaterialIndex를 바 인덱스로, ZOrder를 정규화 높이로 재해석
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\visualizer_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\visualizer_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"AudioVisualizer", shader);
	}

	// CircularVisualizer — 원형 오디오 비주얼라이저
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\circular_vis_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\circular_vis_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"CircularVisualizer", shader);
	}

	// Decal
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_FRONT,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\decal_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\decal_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"Decal", shader);
	}

	// UIHpFragment — HP 손실 파편(삼각형) 전용
	// UIInstanceData를 삼각형 정점 컨테이너로 재해석, UITriangle 메시(3정점)를 인스턴싱
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\ui_hp_fragment_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\ui_hp_fragment_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"UIHpFragment", shader);
	}

	// WorldUIHpSprite — HP 바 배경/채움 (3D 빌보드 + depth occlusion)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\world_ui_hp_sprite_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\world_ui_hp_sprite_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"WorldUIHpSprite", shader);
	}

	// WorldUIHpFragment — HP 손실 파편 (3D 빌보드 + depth occlusion + sprite 알파 마스킹)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\world_ui_hp_fragment_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\world_ui_hp_fragment_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"WorldUIHpFragment", shader);
	}

	// WorldUIHpHit — 피격 순간 경계점에 덧그리는 스프라이트 시트 오버레이 (가산 블렌드)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ONE_TO_ONE_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\world_ui_hp_hit_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\world_ui_hp_hit_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"WorldUIHpHit", shader);
	}

	// WorldUIConquestRing — 점령 진행률 원형 게이지 (라디얼 필 + 도넛 마스크)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::UI,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
			BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\world_ui_conquest_ring_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\world_ui_conquest_ring_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"WorldUIConquestRing", shader);
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
		shader->CreateGraphicsShader(shaderPath, info, 4, "VS_Main", "PS_Main");
		Add<Shader>(L"DebugLine_NoDepth", shader);
	}

	// JHToon (Forward) — 메인 캐릭터/적 툰 셰이딩 PS
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::EQUAL_NO_WRITE

		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\JHToon_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"JHToon", shader);
	}

	// Forward Alpha
	// CULL_NONE     : 풀잎 양면 렌더
	// LESS_EQUAL    : DepthPrepassAlpha가 기록한 깊이에 EQUAL 매치 (ForwardPass DSV는 read-only)
	// ALPHA_TEST    : DepthPrePass에서 DepthPrepassAlpha PSO로 깊이 선기록됨
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_EQUAL,
			BLEND_TYPE::ALPHA_TEST,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_alpha_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\forward_alpha_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"ForwardAlpha", shader);
	}

	// Forward Alpha (식생 알파 컷아웃 전용)
	// CULL_NONE     : 풀잎 양면 렌더
	// LESS_EQUAL    : DepthPrepassAlpha가 기록한 깊이에 EQUAL 매치 (ForwardPass DSV는 read-only)
	// ALPHA_TEST    : DepthPrePass에서 DepthPrepassAlpha PSO로 깊이 선기록됨
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_NONE,
			DEPTH_STENCIL_TYPE::LESS_EQUAL,
			BLEND_TYPE::ALPHA_TEST,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_alpha_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\forward_foliage_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"ForwardFoliage", shader);
	}

	// ForwardBillboard (로고·간판 등 월드 공간 쿼드 전용)                                                                                                                                                                                                                                  1209 +  // ForwardPass가 read-only DSV를 바인딩하므로 depth write 불가 → LESS_NO_WRITE 사용
	// ALPHA_BLEND : 알파 반투명 지원
	{
		ShaderInfo info =
		{
		  SHADER_TYPE::FORWARD,
		  RASTERIZER_TYPE::CULL_NONE,
		  DEPTH_STENCIL_TYPE::LESS_NO_WRITE,
		  BLEND_TYPE::ALPHA_BLEND,
		};
		ShaderPath shaderPath{
		  .VS = L"..\\Resources\\Shader\\forward_alpha_VS.hlsl",
		  .PS = L"..\\Resources\\Shader\\forward_alpha_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"ForwardBillboard", shader);
	}

	// solid_PS (Forward)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::EQUAL_NO_WRITE

		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\solid_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, ShaderArg());
		Add<Shader>(L"Solid", shader);
	}

	// ToneMap
	{
		ShaderInfo info =
		{
			SHADER_TYPE::TONEMAP,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\final_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\tonemap_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Final", "PS_Main");
		Add<Shader>(L"ToneMap", shader);
	}

	// LobbyBackground
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\final_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\lobby_background_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->SetTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Final", "PS_Main");
		Add<Shader>(L"LobbyBackground", shader);
	}

	// ChromaticAberration
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LDRPOST,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\ChromaticAberration_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\ChromaticAberration_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"ChromaticAberration", shader);
	}

	// MotionVector (velocity 계산 → MOTION_VECTOR RT 출력, R16G16B16A16_FLOAT)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\MotionVector_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\MotionVector_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->SetTargetFormat(DXGI_FORMAT_R16G16_FLOAT);
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"MotionVector", shader);
	}

	// MotionBlur (HDR RT에서 blur 적용, R16G16B16A16_FLOAT)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\MotionVector_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\MotionBlur_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"MotionBlur", shader);
	}

	// COMPOSITE
	{
		ShaderInfo info =
		{
			SHADER_TYPE::COMPOSITE,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\FianlComposite_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\FianlComposite_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"FianlComposite", shader);
	}

	// Depth-Prepass
	{
		ShaderInfo info =
		{
			SHADER_TYPE::DEPTH_PREPASS,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::LESS,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\depth_prepass_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\depth_prepass_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"DepthPrepass", shader);
	}

	// Depth-Prepass (알파 컷아웃 식생 전용 — 텍스처 알파 clip 후 depth 기록)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::DEPTH_PREPASS,
			RASTERIZER_TYPE::CULL_NONE,  // 풀잎 양면
			DEPTH_STENCIL_TYPE::LESS,
			BLEND_TYPE::ALPHA_TEST,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\forward_alpha_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\depth_prepass_alpha_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, RENDERMANAGER.GetMsaaSampleCount(), "VS_Main", "PS_Main");
		Add<Shader>(L"DepthPrepassAlpha", shader);
	}

	// Outline (Inverted Hull)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_FRONT,
			DEPTH_STENCIL_TYPE::LESS_EQUAL,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\outline_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\outline_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"Outline", shader);
	}

	// Luminance
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LDRPOST,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\Luminance_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"Luminance", shader);
	}

	// Fog
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\Fog_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"Fog", shader);
	}

	// HBAO+
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};

		// HBAO 메인 계산 (G-Buffer Position/Normal -> AO)
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\hbao_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();

			shader->SetTargetFormat(DXGI_FORMAT_R8_UNORM);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"HBAO", shader);
		}

		// HBAO Cross-bilateral Separable Blur (가로/세로 공용)
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\hbao_blur_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();

			shader->SetTargetFormat(DXGI_FORMAT_R8_UNORM);
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"HBAOBlur", shader);
		}
	}

	// GodRay — Volumetric Light Scattering (레이마칭 + CSM Shadow)
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\godray_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"GodRay", shader);
	}

	// Emissive
	{
		ShaderInfo info =
		{
			SHADER_TYPE::FORWARD,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};

		// Emissive Bright Extract
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\emissive_extract_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"KawaseExtract", shader);
		}

		// Dual Kawase Downsample
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\kawase_down_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"KawaseDown", shader);
		}

		// Dual Kawase Upsample
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\kawase_up_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"KawaseUp", shader);
		}

		// Emissive Bloom Composite
		{
			ShaderPath shaderPath{
				.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
				.PS = L"..\\Resources\\Shader\\emissive_composite_PS.hlsl"
			};
			shared_ptr<Shader> shader = make_shared<Shader>();
			shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
			Add<Shader>(L"KawaseComposite", shader);
		}
	}

	// FXAA 
	{
		ShaderInfo fxaaInfo =
		{
			SHADER_TYPE::LDRPOST,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
			.PS = L"..\\Resources\\Shader\\fxaa_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, fxaaInfo, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"FXAA", shader);
	}

	// HealthVignette
	{
		ShaderInfo info =
		{
			SHADER_TYPE::LDRPOST,
			RASTERIZER_TYPE::CULL_BACK,
			DEPTH_STENCIL_TYPE::NO_DEPTH_TEST_NO_WRITE,
		};
		ShaderPath shaderPath{
			.VS = L"..\\Resources\\Shader\\Luminance_VS.hlsl",
			// 수정사항: 중간 include 파일 없이 통합된 체력 및 버프 비네팅 셰이더를 직접 로드한다.
			.PS = L"..\\Resources\\Shader\\HealthBuffVignette_PS.hlsl"
		};
		shared_ptr<Shader> shader = make_shared<Shader>();
		shader->CreateGraphicsShader(shaderPath, info, 1, "VS_Main", "PS_Main");
		Add<Shader>(L"HealthVignette", shader);
	}
}

void ResourceManager::CreateDefaultMaterial()
{

	// Skybox
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Skybox");
		material->SetTexture(Load<Texture>(L"SkyboxTexture", L"..\\Resources\\Texture\\de_skybox.dds"), DIFFUSEMAP0INDEX);
		Add<Material>(L"Skybox", material);

		// IBL 텍스처 로드
		{
			// Irradiance Map (Diffuse IBL용 큐브맵)                                                                                                                                                                                                                                                                         Load<Texture>(L"IBL_Irradiance", L"..\\Resources\\Texture\\skybox_irradiance.dds");
			Load<Texture>(L"IBL_Irradiance", L"..\\Resources\\Texture\\de_iem.dds");

			// Pre-filtered Env Map (Specular IBL용 큐브맵, mip 포함)
			Load<Texture>(L"IBL_PreFiltered", L"..\\Resources\\Texture\\de_pmrem.dds");

			// BRDF LUT (2D 텍스처)
			Load<Texture>(L"IBL_BrdfLut", L"..\\Resources\\Texture\\skybox2Brdf.dds");
		}
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

	// Final
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Final");
		//material->SetTexture(0, Get<Texture>(L"DiffuseTarget"));
		//material->SetTexture(1, Get<Texture>(L"DiffuseLightTarget"));
		//material->SetTexture(2, Get<Texture>(L"SpecularLightTarget"));
		Add<Material>(L"Final", material);
	}

	//// Terrain
	//{

	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"Terrain");
	//	//material->SetTexture(Load<Texture>(L"HeightMap0", L"..\\Resources\\Terrain\\Asphalt.png"), DIFFUSEMAP0INDEX);
	//	//material->SetTexture(Load<Texture>(L"HeightMap1", L"..\\Resources\\Terrain\\Base_Texture.jpg"), DIFFUSEMAP1INDEX);
	//	material->SetTexture(Load<Texture>(L"T_Height", L"..\\Resources\\Terrain\\T_Height.png"), DIFFUSEMAP2INDEX);
	//	Add<Material>(L"Terrain", material);
	//}
	//// Terrain 1
	//{

	//	shared_ptr<Material> color1 = make_shared<Material>();
	//	color1->SetShader(L"Terrain");
	//	color1->SetTexture(Load<Texture>(L"T_Sand_Rock_BC", L"..\\Resources\\Terrain\\T_Sand_Rock_BC.png"), DIFFUSEMAP0INDEX);
	//	color1->SetTexture(Load<Texture>(L"T_Sand_Rock_Layer", L"..\\Resources\\Terrain\\T_Sand_Rock_Layer.png"), DIFFUSEMAP1INDEX);
	//	color1->SetTexture(Load<Texture>(L"Noise0", L"..\\Resources\\Terrain\\T_TilingNoise02_M.png"), DIFFUSEMAP2INDEX);
	//	color1->SetTexture(Load<Texture>(L"colors", L"..\\Resources\\Terrain\\Geom_Rock_Overgrown_B_LOD00_Rock_Overgrown_B_0_Normal.png"), NORMALMAPINDEX);
	//	Add<Material>(L"Sand_Rock", color1);


	//	//
	//	shared_ptr<Material> color2 = make_shared<Material>();
	//	color2->SetShader(L"Terrain");
	//	color2->SetTexture(Load<Texture>(L"T_Dirt_BC", L"..\\Resources\\Terrain\\T_Dirt_BC.png"), DIFFUSEMAP0INDEX);
	//	color2->SetTexture(Load<Texture>(L"T_Dirt_Layer", L"..\\Resources\\Terrain\\T_Dirt_Layer.png"), DIFFUSEMAP1INDEX);
	//	color1->SetTexture(Load<Texture>(L"Noise1", L"..\\Resources\\Terrain\\T_TilingNoise02_M.png"), DIFFUSEMAP2INDEX);
	//	color2->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	Add<Material>(L"Dirt", color2);

	//	//
	//	shared_ptr<Material> color3 = make_shared<Material>();
	//	color3->SetShader(L"Terrain");
	//	color3->SetTexture(Load<Texture>(L"T_Grass_BC", L"..\\Resources\\Terrain\\T_Grass_BC.png"), DIFFUSEMAP0INDEX);
	//	color3->SetTexture(Load<Texture>(L"T_Grass_Layer", L"..\\Resources\\Terrain\\T_Grass_Layer.png"), DIFFUSEMAP1INDEX);
	//	color1->SetTexture(Load<Texture>(L"Noise2", L"..\\Resources\\Terrain\\T_TilingNoise02_M.png"), DIFFUSEMAP2INDEX);
	//	color3->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	Add<Material>(L"Grass", color3);

	//	shared_ptr<Material> color4 = make_shared<Material>();
	//	color4->SetShader(L"Terrain");
	//	color4->SetTexture(Load<Texture>(L"T_Sand_BC", L"..\\Resources\\Terrain\\T_Sand_BC.png"), DIFFUSEMAP0INDEX);
	//	color4->SetTexture(Load<Texture>(L"T_Sand_Layer", L"..\\Resources\\Terrain\\T_Sand_Layer.png"), DIFFUSEMAP1INDEX);
	//	color1->SetTexture(Load<Texture>(L"Noise3", L"..\\Resources\\Terrain\\T_TilingNoise02_M.png"), DIFFUSEMAP2INDEX);
	//	color4->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	Add<Material>(L"Sand", color4);

	//	shared_ptr<Material> color5 = make_shared<Material>();
	//	color5->SetShader(L"Terrain");
	//	color5->SetTexture(Load<Texture>(L"T_Dirt_Road_BC", L"..\\Resources\\Terrain\\T_Dirt_Road_BC.png"), DIFFUSEMAP0INDEX);
	//	color5->SetTexture(Load<Texture>(L"T_Dirt_Road_Layer", L"..\\Resources\\Terrain\\T_Dirt_Road_Layer.png"), DIFFUSEMAP1INDEX);
	//	color1->SetTexture(Load<Texture>(L"Noise4", L"..\\Resources\\Terrain\\T_TilingNoise02_M.png"), DIFFUSEMAP2INDEX);
	//	color5->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	Add<Material>(L"Dirt_Road", color5);

	//	//shared_ptr<Material> color3 = make_shared<Material>();
	//	//color3->SetShader(L"Terrain");
	//	//color3->SetTexture(Load<Texture>(L"T_Ground_Gravel_BC", L"..\\Resources\\Terrain\\T_Ground_Gravel_BC.png"), DIFFUSEMAP0INDEX);
	//	//color3->SetTexture(Load<Texture>(L"Ground_Gravel", L"..\\Resources\\Terrain\\Ground_Gravel.png"), DIFFUSEMAP1INDEX);
	//	//color3->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	//Add<Material>(L"Ground_Gravel", color3);

	//	//shared_ptr<Material> color4 = make_shared<Material>();
	//	//color4->SetShader(L"Terrain");
	//	//color4->SetTexture(Load<Texture>(L"T_Mosaic_BC", L"..\\Resources\\Terrain\\T_Mosaic_BC.png"), DIFFUSEMAP0INDEX);
	//	//color4->SetTexture(Load<Texture>(L"Mosaic", L"..\\Resources\\Terrain\\Mosaic.png"), DIFFUSEMAP1INDEX);
	//	//color4->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	//Add<Material>(L"Mosaic", color4);

	//	//shared_ptr<Material> color5 = make_shared<Material>();
	//	//color5->SetShader(L"Terrain");
	//	//color5->SetTexture(Load<Texture>(L"T_SnowFootprints_BC", L"..\\Resources\\Terrain\\T_SnowFootprints_BC.png"), DIFFUSEMAP0INDEX);
	//	//color5->SetTexture(Load<Texture>(L"SnowFootprints", L"..\\Resources\\Terrain\\SnowFootprints.png"), DIFFUSEMAP1INDEX);
	//	//color5->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	//Add<Material>(L"SnowFootprints", color5);

	//	//shared_ptr<Material> color6 = make_shared<Material>();
	//	//color6->SetShader(L"Terrain");
	//	//color6->SetTexture(Load<Texture>(L"T_Soil_Mud", L"..\\Resources\\Terrain\\T_Soil_Mud.png"), DIFFUSEMAP0INDEX);
	//	//color6->SetTexture(Load<Texture>(L"Soil_Mud", L"..\\Resources\\Terrain\\Soil_Mud.png"), DIFFUSEMAP1INDEX);
	//	//color6->SetTexture(Get<Texture>(L"colors"), NORMALMAPINDEX);
	//	//Add<Material>(L"Soil_Mud", color6);


	//}	// Terrain

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

		material->SetTexture(Load<Texture>(L"ParticleDebugTex", L"..\\Resources\\Image\\UI\\fire.png"), DIFFUSEMAP0INDEX);
		Add<Material>(L"Particle", material);
	}

	// SmokeRise
	{
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"SmokeParticle");
			material->SetTexture(Load<Texture>(L"SmokeParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_CloudsNoise_2.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"SmokeParticle", material);
		}

		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"SmokeParticleInstanced");
			material->SetTexture(Load<Texture>(L"SmokeParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_CloudsNoise_2.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"SmokeParticleInstanced", material);
		}
	}

	// PlazaCloudDrift
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"CloudParticleInstanced");
		material->SetTexture(Load<Texture>(L"CloudParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_CloudsNoise_2.PNG"), DIFFUSEMAP0INDEX);
		Add<Material>(L"CloudParticleInstanced", material);
	}

	// AuraRise
	{
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"AuraParticle");
			material->SetTexture(Load<Texture>(L"AuraParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"AuraParticle", material);
		}
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"AuraParticleInstanced");
			material->SetTexture(Load<Texture>(L"AuraParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"AuraParticleInstanced", material);
		}
	}

	// OrbitParticle
	{
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"OrbitParticle");
			material->SetTexture(Load<Texture>(L"OrbitParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"OrbitParticle", material);
		}
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"OrbitParticleInstanced");
			material->SetTexture(Load<Texture>(L"OrbitParticleNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"OrbitParticleInstanced", material);
		}
	}

	// BulletTrail
	{
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"BulletTrailParticle");
			material->SetTexture(Load<Texture>(L"BulletTrailNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"BulletTrailParticle", material);
		}
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(L"BulletTrailParticleInstanced");
			material->SetTexture(Load<Texture>(L"BulletTrailNoiseTex", L"..\\Resources\\Image\\Noise\\T_FirefliesNoise.PNG"), DIFFUSEMAP0INDEX);
			Add<Material>(L"BulletTrailParticleInstanced", material);
		}
	}

	// ParticleInstancing (Fire)
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"ParticleInstanced");
		material->SetTexture(Load<Texture>(L"ParticleDebugTex", L"..\\Resources\\Image\\UI\\fire.png"), DIFFUSEMAP0INDEX);
		Add<Material>(L"ParticleInstanced", material);
	}


	//HPBAR
	{
		shared_ptr<Texture> texture = Load<Texture>(L"Loading", L"..\\Resources\\Image\\UI\\UI_Rudwig_HP_1.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"HPBAR", material);
	}

	//fire
	{
		shared_ptr<Texture> texture = Load<Texture>(L"fire", L"..\\Resources\\Image\\UI\\fire.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"fire", material);
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
		shared_ptr<Texture> textures = Load<Texture>(L"jAims2", L"..\\Resources\\Image\\UI\\UI_Aim_02.png");
		// 크로스헤어 양옆에서 박자 위치를 보여 주는 좌우 리듬바 텍스처를 등록한다.
		Load<Texture>(L"UI_Aim_Bar_Left", L"..\\Resources\\Image\\UI\\UI_Aim_Bar_Left.png");
		Load<Texture>(L"UI_Aim_Bar_Right", L"..\\Resources\\Image\\UI\\UI_Aim_Bar_Right.png");
		// 박자 판정과 실제 타격 결과에 따라 기존 UI_Aim_02 대신 표시할 조준 UI를 등록한다.
		Load<Texture>(L"UI_Aim_Perfect", L"..\\Resources\\Image\\UI\\UI_Aim_Perfect.png");
		Load<Texture>(L"UI_Aim_Good", L"..\\Resources\\Image\\UI\\UI_Aim_Good.png");
		Load<Texture>(L"UI_Aim_Miss", L"..\\Resources\\Image\\UI\\UI_Aim_Miss.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"UI");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);

		Add<Material>(L"jAims", material);
	}

	//궁극기 컷인
	{
		Load<Texture>(L"UI_White", L"..\\Resources\\Image\\UI\\UI_White.png");

		Load<Texture>(L"UI_Rudwig_Cutin",       L"..\\Resources\\Image\\UI\\UI_Rudwig_Cutin.png");
		Load<Texture>(L"UI_Rudwig_Cutin_Back",  L"..\\Resources\\Image\\UI\\UI_Rudwig_Cutin_Back.png");
		Load<Texture>(L"UI_Ibanix_Cutin",       L"..\\Resources\\Image\\UI\\UI_Ibanix_Cutin.png");
		Load<Texture>(L"UI_Ibanix_Cutin_Back",  L"..\\Resources\\Image\\UI\\UI_Ibanix_Cutin_Back.png");
		Load<Texture>(L"UI_Fanthor_Cutin",      L"..\\Resources\\Image\\UI\\UI_Fanthor_Cutin.png");
		Load<Texture>(L"UI_Fanthor_Cutin_Back", L"..\\Resources\\Image\\UI\\UI_Fanthor_Cutin_Back.png");
		Load<Texture>(L"UI_Boss_Cutin_Back",         L"..\\Resources\\Image\\UI\\UI_Boss_Cutin_Back.png");
		Load<Texture>(L"UI_Boss_Cutin_Tubaitan",     L"..\\Resources\\Image\\UI\\UI_Boss_Cutin_Tubaitan.png");
		Load<Texture>(L"UI_Boss_Cutin_Tubaitan_Img", L"..\\Resources\\Image\\UI\\UI_Boss_Cutin_Tubaitan_Img.png");
		Load<Texture>(L"UI_Boss_Cutin_Violagon",     L"..\\Resources\\Image\\UI\\UI_Boss_Cutin_Violagon.png");
		Load<Texture>(L"UI_Boss_Cutin_Violagon_Img", L"..\\Resources\\Image\\UI\\UI_Boss_Cutin_Violagon_Img.png");
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

	////Title_Background
	//{
	//	shared_ptr<Texture> texture = Load<Texture>(L"Title_Background", L"..\\Resources\\Image\\UI\\UI_Main_Title.png");
	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"UI");
	//	material->SetTexture(texture, DIFFUSEMAP0INDEX);

	//	Add<Material>(L"Title_Background", material);
	//}

	////Ingame_Background
	//{
	//	shared_ptr<Texture> texture = Load<Texture>(L"UI_Ingame_Back", L"..\\Resources\\Image\\UI\\UI_Ingame_Back_01.png");
	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"UI");
	//	material->SetTexture(texture, DIFFUSEMAP0INDEX);

	//	Add<Material>(L"Ingame_Back", material);
	//}

	////Game_Loading_Background
	//{
	//	shared_ptr<Texture> texture = Load<Texture>(L"Game_Loading_Background", L"..\\Resources\\Image\\UI\\UI_Fanthor_Loading.png");
	//	shared_ptr<Material> material = make_shared<Material>();
	//	material->SetShader(L"UI");
	//	material->SetTexture(texture, DIFFUSEMAP0INDEX);

	//	Add<Material>(L"Game_Loading_Background", material);
	//}


	// Ocean Material
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Ocean");
		auto& p = material->GetParams();
		// foamEdgeWidth, foamSoftness, scrollSpeed, flowStrength
		p.ExtValue[0] = Vec4(2.0f, 0.10f, 0.12f, 0.15f);
		// shallowColor.rgb, shallowAlpha
		p.ExtValue[1] = Vec4(0.15f, 0.78f, 0.72f, 0.55f);
		// deepColor.rgb, deepAlpha
		p.ExtValue[2] = Vec4(0.02f, 0.28f, 0.42f, 0.92f);
		// texTiling, depthFadeScale, foamBrightness, surfaceFoamAmount(표면 whitecap 강도)
		p.ExtValue[3] = Vec4(500.f, 8.0f, 1.0f, 0.7f);
		// 거품 노이즈
		shared_ptr<Texture> foamTex = Load<Texture>(L"OceanFoamNoise", L"..\\Resources\\Texture\\water_Normal_0.png");
		shared_ptr<Texture> dirTex = Load<Texture>(L"T_GrassWindNoise", L"..\\Resources\\Image\\Noise\\T_WorlyNoise_3.png");
		shared_ptr<Texture> waterNrm = Load<Texture>(L"OceanWaterNormal", L"..\\Resources\\Texture\\water_Normal_0.png");
		

		p.ExtTex[0] = static_cast<int32>(foamTex->GetImageIndex()); // 거품 노이즈
		p.ExtTex[1] = static_cast<int32>(dirTex->GetImageIndex()); // 거품 노이즈
		p.ExtTex[2] = static_cast<int32>(waterNrm->GetImageIndex()); // 물 노멀맵

		Add<Material>(L"Ocean", material);
	}

	// DebugLine Material
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"DebugLine");
		Add<Material>(L"DebugLine", material);
	}

	// DebugLine_NoDepth Material (항상 보이는 흰색 라인)
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"DebugLine_NoDepth");
		material->GetParams().Diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
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

	{
		// DebugLine_Yellow
		auto mat = make_shared<Material>();
		mat->SetShader(L"DebugLine");
		mat->GetParams().Diffuse = Vec4(1.f, 1.f, 0.f, 1.f);
		Add<Material>(L"DebugLine_Yellow", mat);
	}

	{
		shared_ptr<Texture> texture = Load<Texture>(L"UI_Logo", L"..\\Resources\\Image\\UI\\UI_Logo.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"ForwardAlpha");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);
		material->GetParams().Diffuse = Vec4(1.f, 1.f, 1.f, 1.f);  // ← 이 줄 추가
		Add<Material>(L"UI_Logo", material);
	}


	{
		//auto Tex = Load<Texture>(L"GradientTex", L"..\\Resources\\Texture\\GradientTex.png");
		auto Noise = Load<Texture>(L"NoiseTex", L"..\\Resources\\Image\\Noise\\T_TilingNoise02_M.png");
		// 컬러 그레이딩 LUT
		//Load<Texture>(L"ColorLUT", L"..\\Resources\\Texture\\Cinematic_strip.dds");
		// DashSmoke
		Load<Texture>(L"DashSmoke", L"..\\Resources\\Image\\Noise\\T_CloudsNoise_2.PNG");
		Load<Texture>(L"WorlyNoise", L"..\\Resources\\Image\\Noise\\T_WorlyNoise_3.PNG");
		// 화면 외곽 버프 피드백에 사용하는 2행 2열 아이콘 시트다.
		Load<Texture>(L"UI_Effect_Sheet", L"..\\Resources\\Image\\UI\\UI_Effect_Sheet.png");



		Load<Texture>(L"UI_Title_Control", L"..\\Resources\\Image\\UI\\UI_Title_Control.png");
		// Control 안내 화면은 공통 조작법과 캐릭터별 설명을 포함한 4개 페이지로 구성한다.
		Load<Texture>(L"UI_Title_Control_0", L"..\\Resources\\Image\\UI\\UI_Title_Control_0.png");
		Load<Texture>(L"UI_Title_Control_1", L"..\\Resources\\Image\\UI\\UI_Title_Control_1.png");
		Load<Texture>(L"UI_Title_Control_2", L"..\\Resources\\Image\\UI\\UI_Title_Control_2.png");
		Load<Texture>(L"UI_Title_Control_3", L"..\\Resources\\Image\\UI\\UI_Title_Control_3.png");
		Load<Texture>(L"UI_Title_QuitGame", L"..\\Resources\\Image\\UI\\UI_Title_QuitGame.png");
		Load<Texture>(L"UI_Title_Search", L"..\\Resources\\Image\\UI\\UI_Title_Search.png");
		Load<Texture>(L"UI_Title_SelectFrame", L"..\\Resources\\Image\\UI\\UI_Title_SelectFrame.png");
		Load<Texture>(L"UI_Title_Setting", L"..\\Resources\\Image\\UI\\UI_Title_Setting.png");
		// 메인 메뉴 버튼은 이 타이틀 버튼 아틀라스에서 필요한 영역을 잘라 사용한다.
		Load<Texture>(L"UI_Title_Sheet_01", L"..\\Resources\\Image\\UI\\UI_Title_Sheet_01.png");
		// Setting 항목 상자와 체크박스는 두 번째 타이틀 UI 아틀라스를 사용한다.
		Load<Texture>(L"UI_Title_Sheet_02", L"..\\Resources\\Image\\UI\\UI_Title_Sheet_02.png");

		Load<Texture>(L"UI_Title_PaintSplash_0", L"..\\Resources\\Image\\UI\\UI_Title_PaintSplash_0.png");
		// Load<Texture>(L"UI_Title_Button_Back", L"..\\Resources\\Image\\UI\\UI_Title_Button_Back.png");




		// 세 캐릭터의 초상화 레이어를 하나의 아틀라스에서 잘라 사용한다.
		Load<Texture>(L"UI_Ingame_Sheet", L"..\\Resources\\Image\\UI\\UI_Ingame_Sheet.png");

		Load<Texture>(L"UI_Loading_Main_01", L"..\\Resources\\Image\\UI\\UI_Loading_Main_01.png");
		Load<Texture>(L"UI_Loading_Circle", L"..\\Resources\\Image\\UI\\UI_Loading_Circle.png");
		Load<Texture>(L"UI_Loading_Plaza", L"..\\Resources\\Image\\UI\\UI_Loading_Plaza.png");
		Load<Texture>(L"UI_Loading_FirstGame", L"..\\Resources\\Image\\UI\\UI_Loading_FirstGame.png");
		Load<Texture>(L"UI_Loading_SecondGame", L"..\\Resources\\Image\\UI\\UI_Loading_SecondGame.png");
		Load<Texture>(L"UI_Loading_ThirdGame", L"..\\Resources\\Image\\UI\\UI_Loading_ThirdGame.png");
		Load<Texture>(L"UI_Loading_Circle", L"..\\Resources\\Image\\UI\\UI_Loading_Circle.png");


		Load<Texture>(L"UI_Ingame_Back", L"..\\Resources\\Image\\UI\\UI_Ingame_Back_01.png");
		Load<Texture>(L"UI_NoteMan_Talk_0", L"..\\Resources\\Image\\UI\\UI_NoteMan_Talk_0.png");
		Load<Texture>(L"UI_LevelMan_Talk_0", L"..\\Resources\\Image\\UI\\UI_LevelMan_Talk_0.png");
		Load<Texture>(L"UI_LevelMan_Talk_Level", L"..\\Resources\\Image\\UI\\UI_LevelMan_Talk_Level.png");
		Load<Texture>(L"UI_NoteBoar_Portrait", L"..\\Resources\\Image\\UI\\UI_NoteBoar_Portrait.png");
		Load<Texture>(L"UI_NoteMan_Portrait", L"..\\Resources\\Image\\UI\\UI_NoteMan_Portrait.png");
		Load<Texture>(L"UI_NPC_Portrait_Sheet", L"..\\Resources\\Image\\UI\\UI_NPC_Portrait_Sheet.png");

		// 음향사 파생음악 선택 UI
		Load<Texture>(L"UI_RhythmMan_Background", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Background.png");
		Load<Texture>(L"UI_RhythmMan_Text_Shee", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Text_Shee.png");
		Load<Texture>(L"UI_RhythmMan_Rudwig_Sheet", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Rudwig_Sheet.png");
		Load<Texture>(L"UI_RhythmMan_Rudwig_Line", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Rudwig_Line.png");
		Load<Texture>(L"UI_RhythmMan_Rudwig_Portrait", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Rudwig_Portrait.png");
		Load<Texture>(L"UI_RhythmMan_Ibanix_Sheet", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Ibanix_Sheet.png");
		Load<Texture>(L"UI_RhythmMan_Ibanix_Line", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Ibanix_Line.png");
		Load<Texture>(L"UI_RhythmMan_Ibanix_Portrait", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Ibanix_Portrait.png");
		Load<Texture>(L"UI_RhythmMan_Fanthor_Sheet", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Fanthor_Sheet.png");
		Load<Texture>(L"UI_RhythmMan_Fanthor_Line", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Fanthor_Line.png");
		Load<Texture>(L"UI_RhythmMan_Fanthor_Portrait", L"..\\Resources\\Image\\UI\\UI_RhythmMan_Fanthor_Portrait.png");

     

		Load<Texture>(L"UI_Logo", L"..\\Resources\\Image\\UI\\UI_Logo.png");


		Load<Texture>(L"UI_Rudwig_Paused_Main", L"..\\Resources\\Image\\UI\\UI_Rudwig_Paused_Main.png");
		Load<Texture>(L"UI_Ibanix_Paused_Main", L"..\\Resources\\Image\\UI\\UI_Ibanix_Paused_Main.png");
		Load<Texture>(L"UI_Fanthor_Paused_Main", L"..\\Resources\\Image\\UI\\UI_Fanthor_Paused_Main.png");
		Load<Texture>(L"UI_Rudwig_Paused_Manual", L"..\\Resources\\Image\\UI\\UI_Rudwig_Paused_Manual.png");
		Load<Texture>(L"UI_Ibanix_Paused_Manual", L"..\\Resources\\Image\\UI\\UI_Ibanix_Paused_Manual.png");
		Load<Texture>(L"UI_Fanthor_Paused_Manual", L"..\\Resources\\Image\\UI\\UI_Fanthor_Paused_Manual.png");
		Load<Texture>(L"UI_Rudwig_Paused_Setting", L"..\\Resources\\Image\\UI\\UI_Rudwig_Paused_Setting.png");
		Load<Texture>(L"UI_Ibanix_Paused_Setting", L"..\\Resources\\Image\\UI\\UI_Ibanix_Paused_Setting.png");
		Load<Texture>(L"UI_Fanthor_Paused_Setting", L"..\\Resources\\Image\\UI\\UI_Fanthor_Paused_Setting.png");
		Load<Texture>(L"UI_Rudwig_Paused_Quit", L"..\\Resources\\Image\\UI\\UI_Rudwig_Paused_Quit.png");
		Load<Texture>(L"UI_Ibanix_Paused_Quit", L"..\\Resources\\Image\\UI\\UI_Ibanix_Paused_Quit.png");
		Load<Texture>(L"UI_Fanthor_Paused_Quit", L"..\\Resources\\Image\\UI\\UI_Fanthor_Paused_Quit.png");
		// Pause 루트 메뉴 버튼과 캐릭터별 호버 선을 포함하는 아틀라스다.
		Load<Texture>(L"UI_Paused_Sheet_01", L"..\\Resources\\Image\\UI\\UI_Paused_Sheet_01.png");
		// Pause 화면 상단과 하단에서 반복 이동하는 플레이어별 문구 띠 아틀라스.
		Load<Texture>(L"UI_Paused_Word_Sheet", L"..\\Resources\\Image\\UI\\UI_Paused_Word_Sheet.png");

#pragma region LobbyUI
		{
			Load<Texture>(L"UI_Robby_Sheet", L"..\\Resources\\Image\\UI\\UI_Robby_Sheet.png");
		}
#pragma endregion

#pragma region Display&HPBG
		{
			Load<Texture>(L"UI_SkillIcon_Sheet", L"..\\Resources\\Image\\UI\\UI_SkillIcon_Sheet.png");

			Load<Texture>(L"UI_Fanthor_HP_0", L"..\\Resources\\Image\\UI\\UI_Fanthor_HP_0.png");
			Load<Texture>(L"UI_Rudwig_HP_0", L"..\\Resources\\Image\\UI\\UI_Rudwig_HP_0.png");
			Load<Texture>(L"UI_Ibanix_HP_0", L"..\\Resources\\Image\\UI\\UI_Ibanix_HP_0.png");
		}
#pragma endregion

#pragma region Text
		{

		}
#pragma endregion

#pragma region HPBar
		{
			Load<Texture>(L"UI_Player_HP_3", L"..\\Resources\\Image\\UI\\UI_Player_HP_3.png");
			Load<Texture>(L"UI_Player_Shield", L"..\\Resources\\Image\\UI\\UI_Player_Shield.png");
			// 수정사항: 캐릭터별 뮤트 상태 아이콘을 한 장의 시트로 등록한다.
			Load<Texture>(L"UI_Player_Mute_Sheet", L"..\\Resources\\Image\\UI\\UI_Player_Mute_Sheet.png");
		}
		{
			Load<Texture>(L"UI_Fanthor_HP_0", L"..\\Resources\\Image\\UI\\UI_Fanthor_HP_0.png");
			Load<Texture>(L"UI_Fanthor_HP_1", L"..\\Resources\\Image\\UI\\UI_Fanthor_HP_1.png");
		}
		{
			Load<Texture>(L"UI_Ibanix_HP_0", L"..\\Resources\\Image\\UI\\UI_Ibanix_HP_0.png");
			Load<Texture>(L"UI_Ibanix_HP_1", L"..\\Resources\\Image\\UI\\UI_Ibanix_HP_1.png");
		}
		{
			Load<Texture>(L"UI_Rudwig_HP_0", L"..\\Resources\\Image\\UI\\UI_Rudwig_HP_0.png");
			Load<Texture>(L"UI_Rudwig_HP_1", L"..\\Resources\\Image\\UI\\UI_Rudwig_HP_1.png");
		}

		{
			Load<Texture>(L"UI_Monster_Hp_0", L"..\\Resources\\Image\\UI\\UI_Monster_Hp_0.png");
			Load<Texture>(L"UI_Monster_Hp_1", L"..\\Resources\\Image\\UI\\UI_Monster_Hp_1.png");
			Load<Texture>(L"UI_Monster_Hp_2", L"..\\Resources\\Image\\UI\\UI_Monster_Hp_2.png");

			Load<Texture>(L"UI_Boss_HP_0", L"..\\Resources\\Image\\UI\\UI_Boss_HP_0.png");
			Load<Texture>(L"UI_Boss_HP_1", L"..\\Resources\\Image\\UI\\UI_Boss_HP_1.png");
		}
#pragma endregion


#pragma region InGame
		{

			Load<Texture>(L"UI_Ingame_Prepare", L"..\\Resources\\Image\\UI\\UI_Game_Ready.png");
			Load<Texture>(L"UI_Ingame_Conquest", L"..\\Resources\\Image\\UI\\UI_Ingame_Conquer.png");
			Load<Texture>(L"UI_Ingame_Escort", L"..\\Resources\\Image\\UI\\UI_Ingame_Escort_0.png");
			Load<Texture>(L"UI_Ingame_Fail", L"..\\Resources\\Image\\UI\\UI_Ingame_Fail.png");
			Load<Texture>(L"UI_Ingame_Killboss", L"..\\Resources\\Image\\UI\\UI_Ingame_Killboss_0.png");
			Load<Texture>(L"UI_Ingame_Success", L"..\\Resources\\Image\\UI\\UI_Ingame_Success_0.png");

			// 스테이지 클리어
			Load<Texture>(L"UI_StageClear_0", L"..\\Resources\\Image\\UI\\UI_StageClear_0.png");
			Load<Texture>(L"UI_StageClear_1", L"..\\Resources\\Image\\UI\\UI_StageClear_1.png");
			Load<Texture>(L"UI_StageClear_2", L"..\\Resources\\Image\\UI\\UI_StageClear_2.png");
			Load<Texture>(L"UI_StageClear_Result", L"..\\Resources\\Image\\UI\\UI_StageClear_Result.png");
			Load<Texture>(L"UI_GameClear", L"..\\Resources\\Image\\UI\\UI_GameClear.png");

			// 점수판
			Load<Texture>(L"UI_Result_Combo_Sheet", L"..\\Resources\\Image\\UI\\UI_Result_Combo_Sheet.png");

			// GameOver
			Load<Texture>(L"UI_GameOver_1", L"..\\Resources\\Image\\UI\\UI_GameOver_1.png");
			Load<Texture>(L"UI_GameOver_2", L"..\\Resources\\Image\\UI\\UI_GameOver_2.png");

			Load<Texture>(L"UI_Ingame_Conquest_Info_0", L"..\\Resources\\Image\\UI\\UI_Ingame_Conquest_Info_0.png");
			Load<Texture>(L"UI_Ingame_Conquest_Info_1", L"..\\Resources\\Image\\UI\\UI_Ingame_Conquest_Info_1.png");
			Load<Texture>(L"UI_Ingame_Conquest_Info_2", L"..\\Resources\\Image\\UI\\UI_Ingame_Conquest_Info_2.png");

			Load<Texture>(L"UI_Escort_Info_0", L"..\\Resources\\Image\\UI\\UI_Escort_Info_0.png");
			Load<Texture>(L"UI_Escort_Info_1", L"..\\Resources\\Image\\UI\\UI_Escort_Info_1.png");
			Load<Texture>(L"UI_Escort_Info_2", L"..\\Resources\\Image\\UI\\UI_Escort_Info_2.png");
			Load<Texture>(L"UI_Escort_Info_Cursor", L"..\\Resources\\Image\\UI\\UI_Escort_Info_Cursor.png");
		}
#pragma endregion


#pragma region SAMR
		{

			auto rampTex = RESOURCEMANAGER.Load<Texture>(L"ramp_default", L"..\\Resources\\Texture\\Ramp_Skin.png");
			auto T_Ibanix_Body_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Ibanix_Body_SAMR", L"..\\Resources\\Texture\\T_Ibanix_Body_SAMR.png");
			auto T_Ibanix_Gun_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Ibanix_Gun_SAMR", L"..\\Resources\\Texture\\T_Ibanix_Gun_SAMR.png");

			auto T_Rudwig_Body_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Rudwig_Body_SAMR", L"..\\Resources\\Texture\\T_Rudwig_Body_SAMR.png");
			auto T_Rudwig_Mace_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Rudwig_Mace_SAMR", L"..\\Resources\\Texture\\T_Rudwig_Mace_SAMR.png");

			auto T_Fanthor_Body_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Fanthor_Body_SAMR", L"..\\Resources\\Texture\\T_Fanthor_Body_SAMR.png");
			auto T_Fanthor_Axe_SAMR = RESOURCEMANAGER.Load<Texture>(L"T_Fanthor_Axe_SAMR", L"..\\Resources\\Texture\\T_Fanthor_Axe_SAMR.png");
		}
#pragma endregion


	}

	LoadPayloadPathJson(L"../Resources/Json/BP_Payroad_path_C_2_PayloadPath.json");


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

	LoadFBX(L"..\\Resources\\FBX\\oo1.fbx", L"JHToon");
	//LoadFBX(L"..\\Resources\\FBX\\XYZ.fbx");
	//LoadFBX(L"..\\Resources\\FBX\\ZUP_Ascii_3dmax_Pivot.fbx");
	//LoadFBX(L"..\\Resources\\FBX\\YUP_Ascii_3dmax_Pivot.fbx");

	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Base.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Attack_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Attack_03.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Walk.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Jump.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Land.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_fall.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_BackRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_RightRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_LeftRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Reload.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Skill_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Skill_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Rhythm.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Dance_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Ultimate.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Rudwig\\Anim_Rudwig_Ultimate_Intro.fbx", L"JHToon");



	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Base.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Attack_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Attack_03.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Jump.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_BackRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_RightRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_LeftRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Walk.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Land.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Fall.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Skill_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Skill_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Reload.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Rhythm.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Dance_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Ultimate.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Ibanix\\Anim_Ibanix_Ultimate_Intro.fbx", L"JHToon");



	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Base.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Attack_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Attack_03.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Jump.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Walk.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Land.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Fall.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_BackRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_RightRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_LeftRun.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Reload.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Skill_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Skill_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Rhythm.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Dance_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Ultimate.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Character\\Fanthor\\Anim_Fanthor_Ultimate_Intro.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Noteboar\\SK_NoteBoar_Run.fbx", L"JHToon");

	//mop
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Hornman\\Anim_Hornman_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Hornman\\Anim_Hornman_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Hornman\\Anim_Hornman_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Hornman\\Anim_Hornman_Die.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Bongoman\\Anim_Bongoman_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Bongoman\\Anim_Bongoman_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Bongoman\\Anim_Bongoman_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Bongoman\\Anim_Bongoman_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Bongoman\\Anim_Bongoman_Shield.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Pianoman\\Anim_Pianoman_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Pianoman\\Anim_Pianoman_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Pianoman\\Anim_Pianoman_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Pianoman\\Anim_Pianoman_Die.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Skill_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Skill_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Skill_03.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\BrassBoss\\Anim_BrassBoss_Skill_04.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Run.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Skill_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Skill_02.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Skill_03.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\StringDragon\\Anim_StringDragon_Skill_04.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Mew\\Anim_Mew_Attack_01.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Mew\\Anim_Mew_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Mew\\Anim_Mew_Idle.ani.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Mew\\Anim_Mew_Run.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Monster\\Slime\\Anim_Slime_Attack.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Slime\\Anim_Slime_Die.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Slime\\Anim_Slime_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Slime\\Anim_Slime_Run.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Npc\\Anim_NoteMan_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Npc\\Anim_LevelMan_Idle.fbx", L"JHToon");
	LoadFBX(L"..\\Resources\\FBX\\Npc\\Anim_RhythmMan_Idle.fbx", L"JHToon");

	LoadFBX(L"..\\Resources\\FBX\\Object\\SM_Escort.fbx", L"Deferred");
	LoadFBXModel(L"..\\Resources\\FBX\\Object\\Obelisk.fbx", L"Deferred");
	LoadFBX(L"..\\Resources\\FBX\\Monster\\Obelisk\\SM_Obelisk.fbx", L"JHToon");

	LoadEffect(L"..\\Resources\\Effect\\VFX_Noteboar_dissolve\\vfx_dissolve_NoteBoar.efk");
	LoadEffect(L"..\\Resources\\Effect\\Area\\Jump\\VFX_Sector_Jump.efk");
	LoadEffect(L"..\\Resources\\Effect\\Area\\Heal\\VFX_Sector_Heal.efk");
	LoadEffect(L"..\\Resources\\Effect\\Area\\Spawn\\VFX_Sector_Spawn.efk");
	LoadEffect(L"..\\Resources\\Effect\\Area\\Conquer\\VFX_Sector_Conquer.efk");


	LoadEffect(L"..\\Resources\\Effect\\VFX\\VFX_Ibanix_Hit_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Ibanix_Attack_Hit_01\\VFX_Ibanix_Attack_Hit_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Attack_Hit\\VFX_Fanthor_Attack_Hit.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Rudwig_Attack_Hit\\VFX_Rudwig_Attack_Hit.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Escort_Shockwave\\VFX_Escort_Shockwave.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Escort_Pulse\\VFX_Escort_Pulse.efk");

	LoadEffect(L"..\\Resources\\Effect\\VFX_Ibanix_Bullet\\VFX_Ibanix_Bullet.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Bullet_Slash_1\\VFX_Fanthor_Bullet_Slash_1.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Bullet_Slash_2\\VFX_Fanthor_Bullet_Slash_2.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Bullet_Slash_3\\VFX_Fanthor_Bullet_Slash_3.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Rudwig_Skill_01\\VFX_Rudwig_Skill_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Rudwig_Skill_02\\VFX_Rudwig_Skill_02.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Rudwig_Reload\\VFX_Rudwig_Reload.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Skill_01\\VFX_Fanthor_Skill_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Fanthor_Ultimate\\VFX_Fanthor_Ultimate.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Ibanix_Ultimate\\VFX_Ibanix_Ultimate.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Rudwig_Ultimate\\VFX_Rudwig_Ultimate.efk");

	LoadEffect(L"..\\Resources\\Effect\\VFX_Bongoman_Attack\\VFX_Bongoman_Attack_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Bongoman_Shield\\VFX_Bongoman_Shield.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Pianoman_Attack_01\\VFX_Pianoman_Attack_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Hornman_Bullet\\VFX_Hornman_Bullet.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Monster_Spawn\\VFX_Monster_Spawn.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Player_Rebirth\\VFX_Player_Rebirth.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_BrassBoss_Die\\VFX_BrassBoss_Die.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_BrassBoss_Skill_01\\VFX_BrassBoss_Skill_01.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_BrassBoss_Skill_02\\VFX_BrassBoss_Skill_02.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_BrassBoss_Skill_03\\VFX_BrassBoss_Skill_03.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_BrassBoss_Skill_04\\VFX_BrassBoss_Skill_04.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_Dragon_Skill_04\\VFX_Dragon_Skill_04.efk");


	LoadEffect(L"..\\Resources\\Effect\\UI_TItle.efk");
	LoadEffect(L"..\\Resources\\Effect\\VFX_UI_Select\\VFX_UI_Select.efk");


	LoadNavMesh(L"..\\Resources\\Map\\all_tiles_navmesh.bin");

	Get<Material>(L"Anim_Ibanix_Base0")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Ibanix_Base0")->GetParams().ExtTex[1] = Get<Texture>(L"T_Ibanix_Body_SAMR")->GetImageIndex();

	Get<Material>(L"Anim_Fanthor_Base0")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Fanthor_Base0")->GetParams().ExtTex[1] = Get<Texture>(L"T_Fanthor_Body_SAMR")->GetImageIndex();

	Get<Material>(L"Anim_Rudwig_Base0")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Rudwig_Base0")->GetParams().ExtTex[1] = Get<Texture>(L"T_Rudwig_Body_SAMR")->GetImageIndex();



	//Get<Material>(L"Anim_Fanthor_Base0")->GetParams().ExtTex[1] = Get<Texture>(L"Grayscale")->GetImageIndex();
	//Get<Material>(L"Anim_Rudwig_Base0")->GetParams().ExtTex[1] = Get<Texture>(L"Grayscale")->GetImageIndex();

	Get<Material>(L"Anim_Ibanix_Base1")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Ibanix_Base1")->GetParams().ExtTex[1] = Get<Texture>(L"T_Ibanix_Gun_SAMR")->GetImageIndex();

	Get<Material>(L"Anim_Fanthor_Base1")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Fanthor_Base1")->GetParams().ExtTex[1] = Get<Texture>(L"T_Fanthor_Axe_SAMR")->GetImageIndex();

	Get<Material>(L"Anim_Rudwig_Base1")->GetParams().ExtTex[0] = Get<Texture>(L"ramp_default")->GetImageIndex();
	Get<Material>(L"Anim_Rudwig_Base1")->GetParams().ExtTex[1] = Get<Texture>(L"T_Rudwig_Mace_SAMR")->GetImageIndex();

	// 적 사망 디졸브 노이즈 텍스처 (ExtTex[2])
	// ExtTex[2] = -1로 두면 셰이더가 절차적 노이즈 사용
	int32 dissolveNoiseIdx = Load<Texture>(L"T_Dissolve_MusicMask", L"..\\Resources\\Image\\Noise\\T_Dissolve_MusicWaveMask.png")->GetImageIndex();
	Get<Material>(L"Anim_Hornman_Idle0")->GetParams().ExtTex[2]  = -1;
	Get<Material>(L"Anim_Pianoman_Idle0")->GetParams().ExtTex[2] = -1;
	Get<Material>(L"Anim_Bongoman_Idle0")->GetParams().ExtTex[2] = -1;


	Load<Texture>(L"UI_VFXDecal_Sheet", L"..\\Resources\\Image\\UI\\UI_VFXDecal_Sheet.png");
	Load<Texture>(L"UI_Emote_Sheet", L"..\\Resources\\Image\\UI\\UI_Emote_Sheet.png");
	Load<Texture>(L"DecalSticker", L"..\\Resources\\Image\\UI\\UI_Spray_Sheet.png");

}

void ResourceManager::CreateDefaultParticleEffect()
{
	{
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		// ParticleInstancing
		effect->mDesc.materialName = L"ParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 256;
		effect->mDesc.createInterval = 0.02f;
		effect->mDesc.minLifeTime = 0.8f;
		effect->mDesc.maxLifeTime = 1.6f;
		effect->mDesc.minSpeed = 25.f;
		effect->mDesc.maxSpeed = 55.f;
		effect->mDesc.startScale = 18.f;
		effect->mDesc.endScale = 6.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_DebugBurst");
		Add<ParticleEffect>(L"Particle_DebugBurst", effect);
	}

	{
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"ParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 128;
		effect->mDesc.createInterval = 0.03f;
		effect->mDesc.minLifeTime = 0.6f;
		effect->mDesc.maxLifeTime = 1.0f;
		effect->mDesc.minSpeed = 20.f;
		effect->mDesc.maxSpeed = 40.f;
		effect->mDesc.startScale = 10.f;
		effect->mDesc.endScale = 4.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_DebugFollow");
		Add<ParticleEffect>(L"Particle_DebugFollow", effect);
	}

	{
		// SmokeRise
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"SmokeParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeSmokeParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 192;
		effect->mDesc.createInterval = 0.035f;
		effect->mDesc.minLifeTime = 1.8f;
		effect->mDesc.maxLifeTime = 3.2f;
		effect->mDesc.minSpeed = 6.f;
		effect->mDesc.maxSpeed = 24.f;
		effect->mDesc.startScale = 10.f;
		effect->mDesc.endScale = 36.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_SmokeRise");
		Add<ParticleEffect>(L"Particle_SmokeRise", effect);
	}

	{
		// PlazaCloudFar
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"CloudParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeCloudParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 72;
		effect->mDesc.createInterval = 0.18f;
		effect->mDesc.minLifeTime = 38.0f;
		effect->mDesc.maxLifeTime = 58.0f;
		effect->mDesc.minSpeed = 110.0f;
		effect->mDesc.maxSpeed = 190.0f;
		effect->mDesc.startScale = 620.0f;
		effect->mDesc.endScale = 980.0f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_CloudDriftFar");
		Add<ParticleEffect>(L"Particle_CloudDriftFar", effect);
		Add<ParticleEffect>(L"Particle_CloudDrift", effect);
	}

	{
		// PlazaCloudNear
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"CloudParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeCloudParticleNear";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 32;
		effect->mDesc.createInterval = 0.42f;
		effect->mDesc.minLifeTime = 20.0f;
		effect->mDesc.maxLifeTime = 34.0f;
		effect->mDesc.minSpeed = 85.0f;
		effect->mDesc.maxSpeed = 150.0f;
		effect->mDesc.startScale = 900.0f;
		effect->mDesc.endScale = 1350.0f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_CloudDriftNear");
		Add<ParticleEffect>(L"Particle_CloudDriftNear", effect);
	}

	{
		// AuraRise
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"AuraParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeAuraParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 256;
		effect->mDesc.createInterval = 0.018f;
		effect->mDesc.minLifeTime = 1.1f;
		effect->mDesc.maxLifeTime = 2.0f;
		effect->mDesc.minSpeed = 10.f;
		effect->mDesc.maxSpeed = 42.f;
		effect->mDesc.startScale = 7.f;
		effect->mDesc.endScale = 20.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_AuraRise");
		Add<ParticleEffect>(L"Particle_AuraRise", effect);
	}

	{
		// OrbitParticle 
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"OrbitParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeOrbitParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 128;
		effect->mDesc.createInterval = 0.03f;
		effect->mDesc.minLifeTime = 3.0f;
		effect->mDesc.maxLifeTime = 5.0f;
		effect->mDesc.minSpeed = 24.f;
		effect->mDesc.maxSpeed = 52.f;
		effect->mDesc.startScale = 5.f;
		effect->mDesc.endScale = 9.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_OrbitAround");
		Add<ParticleEffect>(L"Particle_OrbitAround", effect);
	}

	{
		// BulletTrail
		shared_ptr<ParticleEffect> effect = make_shared<ParticleEffect>();
		effect->mDesc.materialName = L"BulletTrailParticleInstanced";
		effect->mDesc.computeMaterialName = L"ComputeBulletTrailParticle";
		effect->mDesc.renderMode = ParticleComponent::RenderMode::InstancedQuadBillboard;
		effect->mDesc.maxParticle = 96;
		effect->mDesc.createInterval = 0.006f;
		effect->mDesc.minLifeTime = 0.12f;
		effect->mDesc.maxLifeTime = 0.28f;
		effect->mDesc.minSpeed = 32.f;
		effect->mDesc.maxSpeed = 95.f;
		effect->mDesc.startScale = 5.f;
		effect->mDesc.endScale = 2.f;
		effect->mDesc.loop = true;
		effect->SetName(L"Particle_BulletTrail");
		Add<ParticleEffect>(L"Particle_BulletTrail", effect);
	}
}
