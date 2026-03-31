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
void Material::CreateMaterial(FBXMaterialInfo& fbxMat)
{
	bool test = true;
	std::cout << "Create Materail ID" << GetID() << std::endl;

	mParams.Diffuse =/* fbxMat.MaterialValueInfo.Diffuse;*/Vec4(1.0f,1.0f,1.0f,1.0f);
	mParams.Ambient = /*fbxMat.MaterialValueInfo.Ambient;*/ Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	mParams.Specular = /*fbxMat.MaterialValueInfo.Specular; */ Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	mParams.Emission = fbxMat.MaterialValueInfo.Emission;

	mParams.Metallic = 0;// fbxMat.MaterialValueInfo.Metallic;
	mParams.Roughness = 1.0f; fbxMat.MaterialValueInfo.Roughness;
	mParams.OcclusionMask = fbxMat.MaterialValueInfo.OcclusionMask;
	mParams.AlphaTest = fbxMat.MaterialValueInfo.AlphaTest;


	std::wstring filePath{ L"..\\Resources\\Texture\\" };
	shared_ptr<Texture> texture;

	if (fbxMat.DiffuseMap0Name != "") {
	
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.DiffuseMap0Name),filePath + s2ws(fbxMat.DiffuseMap0Name));
		test = false;
		SetTexture(texture, DIFFUSEMAP0INDEX);
	}

	if (fbxMat.DiffuseMap1Name != "") {

		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.DiffuseMap1Name), filePath + s2ws(fbxMat.DiffuseMap1Name)); test = false;
		SetTexture(texture, DIFFUSEMAP1INDEX);
	}

	if (fbxMat.DiffuseMap2Name != "") {

		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.DiffuseMap2Name), filePath + s2ws(fbxMat.DiffuseMap2Name)); test = false;
	SetTexture(texture, DIFFUSEMAP2INDEX);
	}

	if (fbxMat.DiffuseMap3Name != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.DiffuseMap3Name), filePath + s2ws(fbxMat.DiffuseMap3Name)); test = false;

	SetTexture(texture, DIFFUSEMAP3INDEX);
	}

	if (fbxMat.NormalMapName != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.NormalMapName), filePath + s2ws(fbxMat.NormalMapName)); test = false;

	SetTexture(texture, NORMALMAPINDEX);
	}

	if (fbxMat.SpecularcMapName != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.SpecularcMapName), filePath + s2ws(fbxMat.SpecularcMapName)); test = false;

	SetTexture(texture, SPECULARCMAPINDEX);
	}

	if (fbxMat.EmissiveMapName != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.EmissiveMapName), filePath + s2ws(fbxMat.EmissiveMapName)); test = false;

	SetTexture(texture, EMISSIVEMAPINDEX);
	}

	if (fbxMat.MetallicMapName != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.MetallicMapName), filePath + s2ws(fbxMat.MetallicMapName)); test = false;

	SetTexture(texture, METALLICMAPINDEX);
	}

	if (fbxMat.OcclusionMapName != "") {
		texture = RESOURCEMANAGER.Load<Texture>(s2ws(fbxMat.OcclusionMapName), filePath + s2ws(fbxMat.OcclusionMapName)); test = false;

	SetTexture(texture, OCCLUSIONMAPINDEX);
	}


}
//
//void Material::SetShader(std::wstring shaderID)
//{
//
//	shared_ptr<Shader> shader = RESOURCEMANAGER.Get<Shader>(shaderID);
//	mShaderID = shaderID;
//	
//
//
//}


