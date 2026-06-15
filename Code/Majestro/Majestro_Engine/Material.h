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
	SPECULARCMAPINDEX,
	EMISSIVEMAPINDEX,
	METALLICMAPINDEX,
	OCCLUSIONMAPINDEX,
	MASKMAPINDEX,

	MATERIAL_TEXTURE_COUNT,


};

struct MaterialParams
{
	Vec4 Diffuse{ 1.f, 1.f, 1.f, 1.f };
	Vec4 Ambient{};
	Vec4 Specular{};
	Vec3 Emission{};

	float Metallic{};
	float Roughness{};
	uint32 OcclusionMask{};
	uint32 AlphaTest{};

	int32 DiffuseMap0Index{ -1 };
	int32 DiffuseMap1Index{ -1 };
	int32 DiffuseMap2Index{ -1 };
	int32 DiffuseMap3Index{ -1 };

	int32 NormalMapIndex{ -1 };
	int32 SpecularcMapIndex{ -1 };
	int32 EmissiveMapIndex{ -1 };
	int32 MetallicMapIndex{ -1 };
	int32 RoughnessMapIndex{ -1 };
	int32 OcclusionMapIndex{ -1 };
	int32 MaskMapIndex{ -1 };

	int32 ExtTex[8]{ -1,-1,-1,-1,-1,-1,-1,-1 }; // 머티리얼별 자유 텍스처 슬롯 8개 (-1 = 미사용)
	Vec4  ExtValue[4]{};                          // 머티리얼별 자유 파라미터 슬롯 4개
};

struct MaterialParamsName
{

	wstring DiffuseMap0Index{};
	wstring DiffuseMap1Index{};
	wstring DiffuseMap2Index{};
	wstring DiffuseMap3Index{};

	wstring NormalMapIndex{  };
	wstring SpecularcMapIndex{ };
	wstring EmissiveMapIndex{ };
	wstring MetallicMapIndex{ };
	wstring RoughnessMapIndex{ };
	wstring OcclusionMapIndex{ };
	wstring MaskMapIndex{ };

};

class Material : public Object
{

public:
	Material();
	virtual ~Material();

	shared_ptr<Shader>& GetShader() { return mShader; }
	wstring& GetShaderName() { return mShaderName; }
	uint32 GetShaderID() { return mShaderID; }
	void SetShader(std::wstring name);

	void SetTexture(shared_ptr<Texture> texture, uint8 texturetype);
	shared_ptr<Texture> GetTexture(uint8 texturetype) { return mTextures[texturetype]; }


	shared_ptr<Material> Clone();
	MaterialParams& GetParams() { return mParams; }
	MaterialParamsName& GetParamsName() { return mParamsName; }

	uint32 GetIndex() { return mStructuredBufferIndex; }
	void SetIndex(uint32 index) { mStructuredBufferIndex = index; }
	
	void CreateMaterial(struct FBXMaterialInfo&, const wstring& prefix = L"");

private:
	wstring				mShaderName;
	uint32				mShaderID;
	uint32				mStructuredBufferIndex{};
	shared_ptr<Shader>	mShader;
	MaterialParams			mParams{};	//머테리얼 parm
	MaterialParamsName		mParamsName{};	//머테리얼 parm
	array<shared_ptr<Texture>, MATERIAL_TEXTURE_COUNT> mTextures;	//텍스쳐들

public:
	friend class FBXData;
};


