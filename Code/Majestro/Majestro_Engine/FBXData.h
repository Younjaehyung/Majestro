#pragma once
#include "Object.h"
class CollisionMesh;
static string ReadString(std::ifstream& file);

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

struct FBXFileHeader
{
	uint32 MeshCount = 0;                     // 메시 개수
	uint32 BoneCount = 0;                     // 본 개수
	uint32 AnimClipCount = 0;                 // 애니메이션 클립 개수

};

struct  FBXMaterialValue {

	Vec4 Diffuse{};
	Vec4 Ambient{};
	Vec4 Specular{};
	Vec3 Emission{};

	float Metallic{};
	float Roughness{};
	uint32 OcclusionMask{};
	uint32 AlphaTest{};
};

struct  FBXMaterialInfo
{

	FBXMaterialValue MaterialValueInfo{};


	string ShaderName{};
	string DiffuseMap0Name{};
	string DiffuseMap1Name{};
	string DiffuseMap2Name{};
	string DiffuseMap3Name{};

	string NormalMapName{};
	string SpecularcMapName{};
	string EmissiveMapName{};
	string MetallicMapName{};
	string OcclusionMapName{};
};

struct FBXMeshInfo
{
	// uint32 NameLength;                        // 이름 길이 -> writeString
	uint32 VertexCount;                      // 정점 개수
	uint32 MaterialCount;                    // 머티리얼 개수
	uint32 HasAnimation;                     // 애니메이션 여부 (bool을 uint32로)
};

struct FBXBMeshInfo
{
	string								Name;
	vector<Vertex>						Vertices;
	vector<vector<uint32>>				Indices;
	vector<FBXMaterialInfo>				Materials;
	//vector<BoneWeight>				BoneWeights; // 사용 안함
	bool								hasAnimation;
};


struct FBXBoneInfo
{
	string		BoneName;
	int32		ParentIndex{};
	XMFLOAT4X4	MatOffset{};                    // 오프셋 매트릭스
};


struct FBXKeyFrameInfo
{
	XMFLOAT4X4 MatTransform = Matrix::Identity;
	double Time{};
};

struct FBXAnimClipInfo
{
	string	Name;
	double StartTime;
	double EndTime;
	uint32 TimeMode;                         // FbxTime::EMode를 uint32로
	vector<vector<FBXKeyFrameInfo>>	KeyFrameInfo{};
};

class FBXData : public Object
{
public:
	
	FBXData();
	virtual ~FBXData();
	
	virtual void Load(const wstring& path, const wstring& shader = L"Deferred");
	void LoadMeshOnly(const wstring& path);
	FBXMaterialInfo ReadMaterialData(std::ifstream& file);
	FBXFileHeader	GetFBXFileHeader() { return mHeader; };
public:
	const vector<shared_ptr<class Mesh>>& GetMeshs() const { return mMeshs; }
	const vector<shared_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	const vector<shared_ptr<CollisionMesh>>& GetColliders() const { return mColliders; }
private:
	vector<shared_ptr<class Material>>& CreateMaterialFromFBX(ifstream& loader, FBXMeshInfo& metaInfo, FBXBMeshInfo& meshInfo, const wstring& shader);
	vector<shared_ptr<class Mesh>>& CreateMeshFromFBX(ifstream& loader, wstring shader = L"Deferred");
	shared_ptr<class Skeleton> CreateSkeletonFromFBX(ifstream& loader);
	vector<shared_ptr<class Animator>>& CreateAnimatorFromFBX(ifstream& loader);
private:


	std::string						mPath;

	FBXFileHeader					mHeader{};
	vector<shared_ptr<Mesh>>		mMeshs;
	vector<shared_ptr<Material>>	mMaterials;
	shared_ptr<Skeleton>			mSkeleton;
	vector<shared_ptr<Animator>>	mAnimators;

	vector<shared_ptr<CollisionMesh>> mColliders;

};

