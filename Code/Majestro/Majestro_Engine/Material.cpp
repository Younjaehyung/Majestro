#include "pch.h"
#include "Material.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Shader.h"

Material::Material() : Object(OBJECT_TYPE::MATERIAL)
{

}

Material::~Material()
{

}


void Material::SetShader(std::wstring name)
{
	{ mShaderName = name; }
	{ mShader = RESOURCEMANAGER.Get<Shader>(name); }
	{ mShaderID = mShader->GetID(); }
	assert(mShader != nullptr);
	

}

void Material::SetTexture(shared_ptr<Texture> mImageMapIndex, uint8 texturetype)
{
	switch (texturetype)
	{
	case DIFFUSEMAP0INDEX:
		mParams.DiffuseMap0Index = mImageMapIndex->GetImageIndex();
		mParamsName.DiffuseMap0Index = mImageMapIndex->GetName();
		break;
	case DIFFUSEMAP1INDEX:
		mParams.DiffuseMap1Index = mImageMapIndex->GetImageIndex();
		mParamsName.DiffuseMap1Index = mImageMapIndex-> GetName();
		break;
	case DIFFUSEMAP2INDEX:
		mParams.DiffuseMap2Index = mImageMapIndex->GetImageIndex();
		mParamsName.DiffuseMap2Index = mImageMapIndex->GetName();
		break;
	case DIFFUSEMAP3INDEX:
		mParams.DiffuseMap3Index = mImageMapIndex->GetImageIndex();
		mParamsName.DiffuseMap3Index = mImageMapIndex->GetName();
		break;
	case NORMALMAPINDEX:
		mParams.NormalMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.NormalMapIndex = mImageMapIndex->GetName();
		break;
	case SPECULARCMAPINDEX:
		mParams.SpecularcMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.SpecularcMapIndex = mImageMapIndex->GetName();
		mParams.RoughnessMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.RoughnessMapIndex = mImageMapIndex->GetName();
		break;
	case EMISSIVEMAPINDEX:
		mParams.EmissiveMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.EmissiveMapIndex = mImageMapIndex->GetName();
		break;
	case METALLICMAPINDEX:
		mParams.MetallicMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.MetallicMapIndex = mImageMapIndex->GetName();
		break;
	case OCCLUSIONMAPINDEX:
		mParams.OcclusionMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.OcclusionMapIndex = mImageMapIndex->GetName();
		break;
	case MASKMAPINDEX:
		mParams.MaskMapIndex = mImageMapIndex->GetImageIndex();
		mParamsName.MaskMapIndex = mImageMapIndex->GetName();
		break;

	default:
		
		break;
	}
	mTextures[texturetype] = mImageMapIndex;
}


shared_ptr<Material> Material::Clone()
{
	shared_ptr<Material> material = make_shared<Material>();
	
	material->SetShader(mShaderName);
	material->mParams = mParams;
	material->mParamsName = mParamsName;
	material->mTextures = mTextures;
	material->mShaderID = mShaderID;
	material->mStructuredBufferIndex = mStructuredBufferIndex;
	material->mShader = mShader;

	return material;
}
void Material::CreateMaterial(FBXMaterialInfo& fbxMat, const wstring& prefix)
{
	bool test = true;
	std::cout << "Create Materail ID" << GetID() << std::endl;


	auto SanitizeColor = [](const Vec4& c)->Vec4    
		{
		const float kEps = 0.001f;
		const bool rgbZero = (c.x + c.y + c.z) < kEps;        // 색이 사실상 검정        
		const bool alphaZero = c.w < kEps;                            // 투명 
		return (rgbZero || alphaZero) ? Vec4(1.0f, 1.0f, 1.0f, 1.0f) : c;
		};


	mParams.Diffuse = SanitizeColor(fbxMat.MaterialValueInfo.Diffuse);
	mParams.Ambient = SanitizeColor(fbxMat.MaterialValueInfo.Ambient);
	mParams.Specular = SanitizeColor(fbxMat.MaterialValueInfo.Specular);
	mParams.Emission = fbxMat.MaterialValueInfo.Emission;

	mParams.Metallic =  fbxMat.MaterialValueInfo.Metallic;


	// [임시]
	constexpr float kDefaultRoughness = 0.85f;
	float roughness = fbxMat.MaterialValueInfo.Roughness;
	if (fbxMat.SpecularcMapName.empty())
		roughness = kDefaultRoughness;
	mParams.Roughness = roughness;

	mParams.OcclusionMask = fbxMat.MaterialValueInfo.OcclusionMask;
	mParams.AlphaTest = fbxMat.MaterialValueInfo.AlphaTest;


	const std::wstring flatDir{ L"..\\Resources\\Texture\\" };
	const std::wstring subDir = prefix.empty() ? flatDir : (flatDir + prefix + L"\\");

	auto LoadTex = [&](const std::string& texName, uint8 slot)
	{
		if (texName.empty())
			return;
		std::wstring n = s2ws(texName);
		std::wstring ddsName = std::filesystem::path(n).replace_extension(L".dds").wstring();
		bool useSub = !prefix.empty() &&
			(std::filesystem::exists(subDir + n) || std::filesystem::exists(subDir + ddsName));
		std::wstring key = useSub ? (prefix + L"/" + n) : n;
		std::wstring path = (useSub ? subDir : flatDir) + n;
		shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(key, path);
		test = false;
		SetTexture(texture, slot);
	};

	LoadTex(fbxMat.DiffuseMap0Name, DIFFUSEMAP0INDEX);
	LoadTex(fbxMat.DiffuseMap1Name, DIFFUSEMAP1INDEX);
	LoadTex(fbxMat.DiffuseMap2Name, DIFFUSEMAP2INDEX);
	LoadTex(fbxMat.DiffuseMap3Name, DIFFUSEMAP3INDEX);
	LoadTex(fbxMat.NormalMapName,	NORMALMAPINDEX);
	LoadTex(fbxMat.SpecularcMapName, SPECULARCMAPINDEX);
	LoadTex(fbxMat.EmissiveMapName, EMISSIVEMAPINDEX);
	LoadTex(fbxMat.MetallicMapName, METALLICMAPINDEX);
	LoadTex(fbxMat.OcclusionMapName, OCCLUSIONMAPINDEX);
}

