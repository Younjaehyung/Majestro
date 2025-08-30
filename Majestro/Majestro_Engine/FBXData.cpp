#include "pch.h"
#include "FBXData.h"
#include <fstream>
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Animator.h"
#include "Skeleton.h"



void FileLoader::FBXLoader(const string& path)
{
	mPath = path;

	std::ifstream file (path, std::ios::binary);
	if (!file.is_open())
		return;

	try
	{
		
		uint32	nameLength;
		file.read(reinterpret_cast<char*>(&nameLength), sizeof(uint32));
		file.read(reinterpret_cast<char*>(&mFileName), nameLength);

		// 헤더 읽기
		BinaryFileHeader header;
		file.read(reinterpret_cast<char*>(&header), sizeof(BinaryFileHeader));

		mFbxMeshInfo = {};
		mFbxAnimatorInfo = {};

		//_meshes.reserve(header.MeshCount);
		mFbxBoneInfo.boneWeights.reserve(header.BoneCount);
		mFbxAnimatorInfo.reserve(header.AnimClipCount);

		if (header.MeshCount != 0) {
			// 메시 데이터 읽기
			for (uint32 i = 0; i < header.MeshCount; ++i)
			{
				PreFbxMeshInfo pmi{};
				file.read(reinterpret_cast<char*>(&pmi), sizeof(PreFbxMeshInfo));
				mPreFbxMeshInfo = pmi;

				FbxMeshInfo mi{};
				file.read(reinterpret_cast<char*>(&mi.name), pmi.NameLength);

				mi.vertices.reserve(pmi.VerticesSize);
				file.read(reinterpret_cast<char*>(&mi.vertices), pmi.VerticesSize);

				mi.indices.resize(pmi.IndicesSizeOut);
				for (uint32 i = 0; i < mi.indices.size();  ++i) {
					mi.indices[i].reserve(pmi.IndicesSizeIn);
				}
				
				file.read(reinterpret_cast<char*>(&mi.indices), pmi.IndicesSizeOut);

				mi.indices.reserve(pmi.MaterialsSize);
				file.read(reinterpret_cast<char*>(&mi.materials), pmi.MaterialsSize);



				mFbxMeshInfo=mi;
				// 본 로딩 구현
				// 메시 로딩 구현 (역순으로 읽기)
				// 실제 구현은 WriteMeshData의 역순으로 진행
			}
		}

		if (header.BoneCount != 0) {
			// 본 데이터 읽기
			mFbxBoneInfo.boneWeights.reserve(header.BoneCount);
			for (uint32 i = 0; i < header.BoneCount; ++i)
			{
				BoneWeight bw{};
				file.read(reinterpret_cast<char*>(&bw), sizeof(BoneWeight));
				mFbxBoneInfo.boneWeights.push_back(bw);
				// 본 로딩 구현
			}
		}

		if (header.AnimClipCount !=0) {
			// 애니메이션 클립 데이터 읽기
			for (uint32 i = 0; i < header.AnimClipCount; ++i)
			{
				FbxAnimatorInfo ai{};
				file.read(reinterpret_cast<char*>(&ai), sizeof(FbxAnimatorInfo));
				mFbxAnimatorInfo.push_back(ai);
				// 애니메이션 클립 로딩 구현
			}
		}

		

		file.close();
		return;
	}
	catch (const std::exception& e)
	{
		file.close();
		return;
	}
}



FBXData::FBXData() : Object(OBJECT_TYPE::FBXDATA)
{
}

FBXData::~FBXData()
{
}

void FBXData::Load(const wstring& path)
{
	mFBXRigLoader.FBXLoader(ws2s(path));


	for (int32 i = 0; i < mFBXRigLoader.GetMeshCount(); i++)
	{
		shared_ptr<Mesh> mesh = CreateMeshFromFBX(mFBXRigLoader);

		RESOURCEMANAGER.Add<Mesh>(mesh->GetName(), mesh);
		mMesh = mesh;


		for (size_t j = 0; j < mFBXRigLoader.GetMesh(i).materials.size(); j++)
		{
			shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(mFBXRigLoader.GetMesh(i).materials[j].name);
			mMaterials.push_back(material);
		}
	}

	shared_ptr<Skeleton> skeleton = CreateSkeletonFromFBX(mFBXRigLoader);
	RESOURCEMANAGER.Add<Skeleton>(skeleton->GetName(), skeleton);
	mSkeleton = skeleton;
}


void FBXData::LoadAnimation(const string& rigpath, const string& anipath)
{
	
	Load(s2ws(rigpath));
	mFBXAniLoader.FBXLoader(anipath);

	if (anipath == "") {
		return;
	}

	vector<shared_ptr<Animator>> animator;
	animator = CreateAnimatorFromFBX(mFBXAniLoader);

	for (auto& anim : animator) {
		RESOURCEMANAGER.Add<Animator>(anim->GetName(), anim);
		mAnimators.push_back(anim);
	}
}



shared_ptr<Mesh> FBXData::CreateMeshFromFBX(FileLoader& loader)
{
	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->CreateVertexBuffer(meshInfo->vertices);

	for (const vector<uint32>& buffer : meshInfo->indices)
	{
		if (buffer.empty())
		{
			// FBX ������ �̻��ϴ�. IndexBuffer�� ������ ���� ���ϱ� �ӽ� ó��
			vector<uint32> defaultBuffer{ 0 };
			mesh->CreateIndexBuffer(defaultBuffer);
		}
		else
		{
			mesh->CreateIndexBuffer(buffer);
		}
	}

	return mesh;
}

shared_ptr<Skeleton> FBXData::CreateSkeletonFromFBX(FileLoader& loader)
{
	shared_ptr<Skeleton> skeleton = make_shared<Skeleton>();
	skeleton->CreateBones(loader);

	return skeleton;

}

vector<shared_ptr<Animator>> FBXData::CreateAnimatorFromFBX(FileLoader& loader)
{
	vector<shared_ptr<Animator>> animators;
	// TO - DO
	shared_ptr<Animator> animator = make_shared<Animator>();
	animator->CreateAnimations(loader);
	animators.push_back(animator);

	return animators;
}
