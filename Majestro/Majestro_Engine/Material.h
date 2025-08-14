#pragma once
#include "Object.h"

class Shader;
class Texture;

enum
{
	DIFFUSEMAP0INDEX,
	DIFFUSEMAP1INDEX,
	DIFFUSEMAP2INDEX,
	DIFFUSEMAP3INDEX,
	NORMALMAPINDEX,
	EMISSIVEMAPINDEX,
	METALLICMAPINDEX,
	OCCLUSIONMAPINDEX,

	MATERIAL_TEXTURE_COUNT,


};

struct MaterialParams
{
	Vec4 Diffuse{};

	Vec3 Emission{};

	float Metallic{};
	float Roughness{};
	uint32 OcclusionMask{};
	uint32 AlphaTest{};

	int32 DiffuseMap0Index{};
	int32 DiffuseMap1Index{};
	int32 DiffuseMap2Index{};
	int32 DiffuseMap3Index{};

	int32 NormalMapIndex{};
	int32 EmissiveMapIndex{};
	int32 MetallicMapIndex{};
	int32 OcclusionMapIndex{};
};

class Material : public Object
{

public:
	Material();
	virtual ~Material();

	shared_ptr<Shader> GetShader() { return mShader; }
	wstring& GetShaderID() { return mShaderID; }
	void SetShader(std::wstring name);

	void SetTexture(shared_ptr<Texture> texture,uint8 texturetype);

	shared_ptr<Material> Clone();
	MaterialParams& GetParams() { return mParams; }

	uint32 GetIndex() { return mStructuredBufferIndex; }
	void SetIndex(uint32 index) { mStructuredBufferIndex = index; }
private:
	wstring				mShaderID;
	uint32				mStructuredBufferIndex{};
	shared_ptr<Shader>	mShader;	//쉐이더 지울 예정
	MaterialParams		mParams{};	//머테리얼 parm
	array<shared_ptr<Texture>, MATERIAL_TEXTURE_COUNT> mTextures;	//텍스쳐들
};


