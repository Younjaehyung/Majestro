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
		break;
	case DIFFUSEMAP1INDEX:
		mParams.DiffuseMap1Index = mImageMapIndex->GetImageIndex();
		break;
	case DIFFUSEMAP2INDEX:
		mParams.DiffuseMap2Index = mImageMapIndex->GetImageIndex();
		break;
	case DIFFUSEMAP3INDEX:
		mParams.DiffuseMap3Index = mImageMapIndex->GetImageIndex();
		break;
	case NORMALMAPINDEX:
		mParams.NormalMapIndex = mImageMapIndex->GetImageIndex();
		break;
	case EMISSIVEMAPINDEX:
		mParams.EmissiveMapIndex = mImageMapIndex->GetImageIndex();
		break;
	case METALLICMAPINDEX:
		mParams.MetallicMapIndex = mImageMapIndex->GetImageIndex();
		break;
	case OCCLUSIONMAPINDEX:
		mParams.OcclusionMapIndex = mImageMapIndex->GetImageIndex();
		break;

	default:
		
		break;
	}

}


shared_ptr<Material> Material::Clone()
{
	shared_ptr<Material> material = make_shared<Material>();
	
	material->SetShader(mShaderName);
	material->mParams = mParams;
	material->mTextures = mTextures;

	return material;
}
void Material::CreateMaterial(FBXMaterialInfo& fbxMat)
{
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


