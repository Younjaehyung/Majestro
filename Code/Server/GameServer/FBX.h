#pragma once
#include "Object.h"

class Mesh;
class CollisionMesh;

static string ReadString(std::ifstream& file);

struct FBXFileHeader
{
	uint32 MeshCount = 0;                     // 메시 개수
	uint32 BoneCount = 0;                     // 본 개수
	uint32 AnimClipCount = 0;                 // 애니메이션 클립 개수
};

struct FBXMeshInfo
{
	// uint32 NameLength;                        // 이름 길이 -> writeString
	uint32 VertexCount;                      // 정점 개수
	uint32 MaterialCount;                    // 머티리얼 개수
	uint32 HasAnimation;                     // 애니메이션 여부 (bool을 uint32로)
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

struct FBXBMeshInfo
{
	string								Name;
	vector<Vertex>						Vertices;
	vector<vector<uint32>>				Indices;
	vector<FBXMaterialInfo>				Materials;
	//vector<BoneWeight>				BoneWeights; // 사용 안함
	bool								hasAnimation;
};

class FBX : public Object
{
public:
	FBX();
	~FBX();
	
	void Load(const wstring& path);
	vector<shared_ptr<Mesh>>& CreateMeshFromFBX(ifstream& loader);
	vector<shared_ptr<CollisionMesh>>& CreateColliderFromFBX(ifstream& loader);

private:

	std::string						mPath;

	FBXFileHeader							mHeader{};
	vector<shared_ptr<Mesh>>				mMeshs;
	vector<shared_ptr<CollisionMesh>>		mColliders;
};

