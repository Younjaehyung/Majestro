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
	shared_ptr<FBXData> meshData = Get<FBXData>(path);
	if (meshData)
		return meshData;
	meshData = make_shared<FBXData>();
	meshData->Load(path);
	meshData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	Add(path, meshData);

	return meshData;
}

void ResourceManager::LoadAllTexture(const wstring& path)
{
	// std::string filePath{ filesystem::path(path).parent_path().string() + "\\" + filesystem::path(path).filename().stem().string() };

}

void ResourceManager::LoadResourceJson(const wstring& path)
{
	//meshData->Load(path);
	//meshData->SetName(s2ws(filesystem::path(path).filename().stem().string()));
	//Add(path, meshData);

}

shared_ptr<Texture> ResourceManager::CreateTexture(const wstring& name, DXGI_FORMAT format, uint32 width, uint32 height,
	const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
	D3D12_RESOURCE_FLAGS resFlags,bool createSRVUAV, Vec4 clearColor)
{
	shared_ptr<Texture> texture = make_shared<Texture>();
	texture->Create(format, width, height, heapProperty, heapFlags, resFlags, createSRVUAV,clearColor);
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
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBUFFER_INDEX_COUNT, 0,0), // b1~b4 몇번부터 몇개까지 레지스터를 사용할건지 작성

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
		rootSignature->AddConstant(0, 2);
		rootSignature->AddTable(ranges0);
		rootSignature->AddTable(ranges1);
		rootSignature->AddTable(ranges2);
		rootSignature->AddTable(ranges3);
		rootSignature->AddTable(ranges4);
		rootSignature->AddSampler(CD3DX12_STATIC_SAMPLER_DESC(0));
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
	

	// Skybox
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

		shader->CreateGraphicsShader(shaderPath, info, ShaderArg());

		Add<Shader>(L"Skybox", shader);
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
		shader->CreateGraphicsShader(shaderPath, info, arg);
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

	//	shared_ptr<Shader> shader = make_shared<Shader>();
	//	shader->CreateGraphicsShader(shaderPath, info);
	//	Add<Shader>(L"Cel", shader);
	//}

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
		shader->CreateGraphicsShader(shaderPath, info, "VS_Main", "PS_Main");
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
		shader->CreateGraphicsShader(shaderPath, info, ShaderArg());
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
		shader->CreateGraphicsShader(shaderPath, info, "VS_Tex", "PS_Tex");
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
		shader->CreateGraphicsShader(shaderPath, info, "VS_DirLight", "PS_DirLight");
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
		shader->CreateGraphicsShader(shaderPath, info, "VS_PointLight", "PS_PointLight");
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
		shader->CreateGraphicsShader(shaderPath, info, "VS_Final", "PS_Final");
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
		shader->CreateGraphicsShader(shaderPath, info, "VS_Main", "PS_Main", "GS_Main");
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
		shader->CreateGraphicsShader(shaderPath, info, ShaderArg());
		Add<Shader>(L"Shadow", shader);
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
}

void ResourceManager::CreateDefaultMaterial()
{
	// Skybox
	{

		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Skybox");
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
		material->SetTexture(Load<Texture>(L"HeightMap0", L"..\\Resources\\Texture\\terrain.png"), DIFFUSEMAP0INDEX);
		material->SetTexture(Load<Texture>(L"HeightMap1", L"..\\Resources\\Texture\\Base_Texture.jpg"), DIFFUSEMAP1INDEX);
		material->SetTexture(Load<Texture>(L"HeightMap2", L"..\\Resources\\Texture\\height.png"), DIFFUSEMAP2INDEX);
		Add<Material>(L"Terrain", material);
	}

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


	LoadFBX(L"..\\Resources\\FBX\\oo1.fbx");
	 LoadFBX(L"..\\Resources\\FBX\\Capoeira.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Dragon.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Rudwig_aIdle_001.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Rudwig_aJump_001.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Rudwig_aRun_001.fbx");
	LoadFBX(L"..\\Resources\\FBX\\Rudwig_aWalk_001.fbx");

}