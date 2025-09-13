#include "pch.h"
#include "FBXData.h"
#include <fstream>
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Animator.h"
#include "Skeleton.h"
#include "Material.h"

/*
*	[STEP0]
		All Texture Load
	[STEP1]
		FBX Load (model Load)
			-> bin to class Resource
	[STEP1-2]
		If Animation exist Load Animation too
	[STEP2]
		Distribute loaded animation files into each class

*/


string ReadString(std::ifstream& file)
{
	uint32 length;
	file.read(reinterpret_cast<char*>(&length), sizeof(uint32));

	if (length == 0)
		return "";

	string utf8Str(length, '\0');
	file.read(&utf8Str[0], length);
	cout<< utf8Str <<endl;
	return utf8Str; // 기존의 s2ws 함수 사용
}

FBXMaterialInfo FBXData::ReadMaterialData(std::ifstream& file)
{

	FBXMaterialInfo m{};
	FBXMaterialValue mv{};
	file.read(reinterpret_cast<char*>(&mv), sizeof(mv));
	m.MaterialValueInfo = mv;

	m.ShaderName = ReadString(file);

	m.DiffuseMap0Name = ReadString(file);
	m.DiffuseMap1Name = ReadString(file);
	m.DiffuseMap2Name = ReadString(file);
	m.DiffuseMap3Name = ReadString(file);

	m.NormalMapName = ReadString(file);
	m.SpecularcMapName = ReadString(file);
	m.EmissiveMapName = ReadString(file);
	m.MetallicMapName = ReadString(file);
	m.OcclusionMapName = ReadString(file);

	return m;
}

//////////////////////////////////////////////////////////////////////////


FBXData::FBXData() : Object(OBJECT_TYPE::FBXDATA)
{
}

FBXData::~FBXData()
{
}

void FBXData::Load(const wstring& path)
{
	mPath = ws2s(path);
	std::string filePath{ filesystem::path(mPath).parent_path().string() + "\\" + filesystem::path(mPath).filename().stem().string()};


	if (std::ifstream f(filePath + ".mesh", std::ios::binary);f) {
		f.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));

		CreateMeshFromFBX(f);
		
	}
	if (std::ifstream f(filePath + ".skel", std::ios::binary); f) {
		CreateSkeletonFromFBX(f);
	}
	if (std::ifstream f(filePath + ".ani", std::ios::binary); f) {
		CreateAnimatorFromFBX(f);
	}


}

vector<shared_ptr<Mesh>>& FBXData::CreateMeshFromFBX(ifstream& loader)
{
	vector<shared_ptr<Mesh>> meshs;
	for (uint8 i = 0; i < mHeader.MeshCount; ++i) {
		shared_ptr<Mesh> mesh = make_shared<Mesh>();

		// === 1) .mesh ===

		string meshName = ReadString(loader);

		FBXMeshInfo metaMeshInfo{};
		loader.read(reinterpret_cast<char*>(&metaMeshInfo), sizeof(metaMeshInfo));

		FBXBMeshInfo meshInfo;

		// Mesh Load
		static_assert(std::is_trivially_copyable_v<Vertex>,
			"Vertex must be trivially copyable");
		meshInfo.Vertices.resize(metaMeshInfo.VertexCount);
		if (metaMeshInfo.VertexCount)
			loader.read(reinterpret_cast<char*>(meshInfo.Vertices.data()),
				sizeof(Vertex) * metaMeshInfo.VertexCount);

		// Indices (by material) Load
		meshInfo.Indices.resize(metaMeshInfo.MaterialCount);
		for (uint32 s = 0; s < metaMeshInfo.MaterialCount; ++s)
		{
			uint32 ic = 0;
			loader.read(reinterpret_cast<char*>(&ic), sizeof(ic));
			meshInfo.Indices[s].resize(ic);
			if (ic)
				loader.read(reinterpret_cast<char*>(meshInfo.Indices[s].data()),
					sizeof(uint32) * ic);
		}
		mesh->SetName(s2ws(meshName));
		mesh->CreateMesh(meshInfo);
		meshs.push_back(mesh);
		RESOURCEMANAGER.Add<Mesh>(mesh->GetName(), mesh);

		CreateMaterialFromFBX(loader, metaMeshInfo, meshInfo);
	}
	loader.close();
	cout << "Materials OVER" << endl;

	return meshs;
}

vector<shared_ptr<Material>>& FBXData::CreateMaterialFromFBX(ifstream& loader, FBXMeshInfo& metaInfo, FBXBMeshInfo& meshInfo)
{
	// Materials

	meshInfo.Materials.reserve(metaInfo.MaterialCount);
	mMaterials.resize(metaInfo.MaterialCount);
	for (uint32 s = 0; s < metaInfo.MaterialCount; ++s) {
		mMaterials[s] =make_shared<Material>();
		mMaterials[s]->SetName(s2ws(ReadString(loader)));
		meshInfo.Materials.push_back(ReadMaterialData(loader));

		mMaterials[s]->CreateMaterial(meshInfo.Materials[s]);
		mMaterials[s]->SetShader(L"Deferred");
		RESOURCEMANAGER.Add<Material>(mMaterials[s]->GetName(), mMaterials[s]);
	}
	
	return mMaterials;
}

shared_ptr<Skeleton> FBXData::CreateSkeletonFromFBX(ifstream& loader)
{
	shared_ptr<Skeleton> skeleton = make_shared<Skeleton>();
	skeleton->mBones.reserve(mHeader.BoneCount);
	struct Dummy {
		int32					parentIdx{};
		XMFLOAT4X4				matOffset{};
	};
	string fileName = ReadString(loader);
	for (uint32 bi = 0; bi < mHeader.BoneCount; ++bi)
	{
		BoneInfo	fbxBondInfo;
		Dummy dummy;
		fbxBondInfo.boneName = ReadString(loader);

		loader.read(reinterpret_cast<char*>(&dummy), sizeof(dummy));
		fbxBondInfo.parentIdx = dummy.parentIdx;
		fbxBondInfo.matOffset = dummy.matOffset; // XMFLOAT4X4 그대로

		skeleton->mBones.emplace_back(fbxBondInfo);
	}
	loader.close();
	RESOURCEMANAGER.Add(s2ws(fileName),skeleton);

	return skeleton;
}

vector<shared_ptr<Animator>>& FBXData::CreateAnimatorFromFBX(ifstream& loader)
{
	mAnimators.resize(mHeader.AnimClipCount);

	for (uint32 ai = 0; ai < mHeader.AnimClipCount; ++ai)
	{
		mAnimators[ai] = make_shared<Animator>();

		FBXAnimClipInfo animClipInfo{};

		struct DummyAnimClipInfo
		{
			// string	Name; -> WriteString
			double StartTime;
			double EndTime;
			uint32 TimeMode;                         // FbxTime::EMode를 uint32로
			//	BinaryKeyFrameInfo	KeyFrameInfo;-> vector<Vector>
		};

		DummyAnimClipInfo dummy{};

		animClipInfo.Name = ReadString(loader);
		loader.read(reinterpret_cast<char*>(&dummy), sizeof(dummy));
		animClipInfo.StartTime = dummy.StartTime;
		animClipInfo.EndTime = dummy.EndTime;
		animClipInfo.TimeMode = dummy.TimeMode;

		// boneTracks
		uint32 boneTracks = 0;
		loader.read(reinterpret_cast<char*>(&boneTracks), sizeof(boneTracks));
		animClipInfo.KeyFrameInfo.resize(boneTracks);

		// 각 본 트랙
		for (uint32 b = 0; b < boneTracks; ++b)
		{
			uint32 kcount = 0;
			loader.read(reinterpret_cast<char*>(&kcount), sizeof(kcount));
			auto& track = animClipInfo.KeyFrameInfo[b];
			track.resize(kcount);

			for (uint32 k = 0; k < kcount; ++k)
			{
				FBXKeyFrameInfo binKF{};
				loader.read(reinterpret_cast<char*>(&binKF), sizeof(binKF));

				binKF.MatTransform;
				binKF.Time;
				track[k] = binKF;
			}
		}

		mAnimators[ai]->SetName(s2ws(animClipInfo.Name));
		mAnimators[ai]->CreateAnimations(loader);
		mAnimators.emplace_back(std::make_shared<Animator>(animClipInfo));
		RESOURCEMANAGER.Add<Animator>(mAnimators[ai]->GetName(), mAnimators[ai]);
	}
	loader.close();

	return mAnimators;
}



////////////////////////////////////////////////////

