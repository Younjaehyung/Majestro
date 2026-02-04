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
	case SPECULARCMAPINDEX:
		mParams.SpecularcMapIndex = mImageMapIndex->GetImageIndex();
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
	mTextures[texturetype] = mImageMapIndex;
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

	std::cout << "Create Materail ID" << GetID() << std::endl;

	mParams.Diffuse = Vec4(1.0f,1.0f,1.0f,1.0f);
	mParams.Ambient = fbxMat.MaterialValueInfo.Ambient;
	mParams.Specular = fbxMat.MaterialValueInfo.Specular;
	mParams.Emission = fbxMat.MaterialValueInfo.Emission;

	mParams.Metallic = fbxMat.MaterialValueInfo.Metallic;
	mParams.Roughness = fbxMat.MaterialValueInfo.Roughness;
	mParams.OcclusionMask = fbxMat.MaterialValueInfo.OcclusionMask;
	mParams.AlphaTest = fbxMat.MaterialValueInfo.AlphaTest;


	std::wstring filePath{ L"..\\Resources\\Texture\\" };
	shared_ptr<Texture> texture;

	if (fbxMat.DiffuseMap0Name != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.DiffuseMap0Name));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.DiffuseMap0Name));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.DiffuseMap0Name), texture);
		}
		SetTexture(texture, DIFFUSEMAP0INDEX);
	}

	if (fbxMat.DiffuseMap1Name != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.DiffuseMap1Name));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.DiffuseMap1Name));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.DiffuseMap1Name), texture);
		}
	SetTexture(texture, DIFFUSEMAP1INDEX);
	}

	if (fbxMat.DiffuseMap2Name != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.DiffuseMap2Name));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.DiffuseMap2Name));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.DiffuseMap2Name), texture);
		}
	SetTexture(texture, DIFFUSEMAP2INDEX);
	}

	if (fbxMat.DiffuseMap3Name != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.DiffuseMap3Name));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.DiffuseMap3Name));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.DiffuseMap3Name), texture);
		}
	SetTexture(texture, DIFFUSEMAP3INDEX);
	}

	if (fbxMat.NormalMapName != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.NormalMapName));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.NormalMapName));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.NormalMapName), texture);
		}
	SetTexture(texture, NORMALMAPINDEX);
	}

	if (fbxMat.SpecularcMapName != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.SpecularcMapName));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.SpecularcMapName));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.SpecularcMapName), texture);
		}
	SetTexture(texture, SPECULARCMAPINDEX);
	}

	if (fbxMat.EmissiveMapName != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.EmissiveMapName));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.EmissiveMapName));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.EmissiveMapName), texture);
		}
	SetTexture(texture, EMISSIVEMAPINDEX);
	}

	if (fbxMat.MetallicMapName != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.MetallicMapName));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.MetallicMapName));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.MetallicMapName), texture);
		}
	SetTexture(texture, METALLICMAPINDEX);
	}

	if (fbxMat.OcclusionMapName != "") {
		texture = RESOURCEMANAGER.Get<Texture>(s2ws(fbxMat.OcclusionMapName));
		if (texture == nullptr) {
			texture = make_shared<Texture>();
			texture->Load(filePath + s2ws(fbxMat.OcclusionMapName));
			RESOURCEMANAGER.Add<Texture>(s2ws(fbxMat.OcclusionMapName), texture);
		}
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


