#pragma once
#include "Object.h"

struct BinaryFileHeader
{
	uint32 MeshCount = 0;                     // 메시 개수
	uint32 BoneCount = 0;                     // 본 개수
	uint32 AnimClipCount = 0;                 // 애니메이션 클립 개수
};

struct BoneWeight
{
	using Pair = pair<int32, double>;
	vector<Pair> boneWeights;

	void AddWeights(uint32 index, double weight)
	{
		if (weight <= 0.f)
			return;

		auto findIt = std::find_if(boneWeights.begin(), boneWeights.end(),
			[=](const Pair& p) { return p.second < weight; });

		if (findIt != boneWeights.end())
			boneWeights.insert(findIt, Pair(index, weight));
		else
			boneWeights.push_back(Pair(index, weight));

		// ����ġ�� �ִ� 4��
		if (boneWeights.size() > 4)
			boneWeights.pop_back();
	}

	void Normalize()
	{
		double sum = 0.f;
		std::for_each(boneWeights.begin(), boneWeights.end(), [&](Pair& p) { sum += p.second; });
		std::for_each(boneWeights.begin(), boneWeights.end(), [=](Pair& p) { p.second = p.second / sum; });
	}
};

struct FbxMaterialInfo
{
	Vec4			diffuse;
	Vec4			ambient;
	Vec4			specular;
	string			name;
	string			diffuseTexName;
	string			normalTexName;
	string			specularTexName;
};

struct PreFbxMeshInfo
{
	uint32								NameLength;
	uint32								VerticesSize;
	uint32								IndicesSizeOut;
	uint32								IndicesSizeIn;
	uint32								MaterialsSize;
	
};

struct FbxMeshInfo
{
	string								name;
	vector<Vertex>						vertices;
	vector<vector<uint32>>				indices;
	vector<FbxMaterialInfo>				materials;
	
};
struct FbxBoneInfo
{
	vector<BoneWeight>					boneWeights; // �� ����ġ
};

struct PreFbxAnimatorInfo
{
	uint32							AnimNameLength;
	uint32							KeyFramesOUT;
	uint32							KeyFramesIN;
};

struct FbxAnimatorInfo
{
	string							animName;
	int32							frameCount;
	double							duration;
	vector<vector<KeyFrameInfo>>	keyFrames;
};

class FileLoader 
{
public:
	void FBXLoader(const string& path);

private:

	string			mPath;
	string			mFileName;

	PreFbxMeshInfo			mPreFbxMeshInfo;
	PreFbxAnimatorInfo		mPreFbxAnimatorInfo;

	FbxMeshInfo		mFbxMeshInfo;
	FbxBoneInfo		mFbxBoneInfo;
	vector<FbxAnimatorInfo>	mFbxAnimatorInfo;
};


class FBXData : public Object
{
public:
	FBXData();
	virtual ~FBXData();

	shared_ptr<Mesh> CreateMeshFromFBX(FileLoader& loader);
	shared_ptr<Skeleton> CreateSkeletonFromFBX(FileLoader& loader);
	vector<shared_ptr<Animator>> CreateAnimatorFromFBX(FileLoader& loader);
	virtual void Load(const wstring& path);
	virtual void LoadAnimation(const string& path, const string& anipath);

private:
	FileLoader						mFBXRigLoader;
	FileLoader						mFBXAniLoader;


	shared_ptr<Mesh>				mMesh;
	vector<shared_ptr<Material>>	mMaterials;

	shared_ptr<Skeleton>			mSkeleton;
	vector<shared_ptr<Animator>>	mAnimators;

};

