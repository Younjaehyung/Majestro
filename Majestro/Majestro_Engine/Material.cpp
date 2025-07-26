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


void Material::PushGraphicsData()
{
	// CBV 업로드
	CONST_BUFFER(CONSTANT_BUFFER_TYPE::MATERIAL)->PushGraphicsData(&mParams, sizeof(mParams));

	// SRV 업로드
	for (size_t i = 0; i < mTextures.size(); i++)
	{
		if (mTextures[i] == nullptr)
			continue;

		SRV_REGISTER reg = SRV_REGISTER(static_cast<int8>(SRV_REGISTER::t0) + i);
		Graphics_DescHeap->SetSRV(mTextures[i]->GetSRVHandle(), reg);
	}

	// 파이프라인 세팅
	mShader->Update();
}

void Material::PushComputeData()
{
	//// CBV 업로드
	//CONST_BUFFER(CONSTANT_BUFFER_TYPE::MATERIAL)->PushComputeData(&mParams, sizeof(mParams));

	//// SRV 업로드
	//for (size_t i = 0; i < mTextures.size(); i++)
	//{
	//	if (mTextures[i] == nullptr)
	//		continue;

	//	SRV_REGISTER reg = SRV_REGISTER(static_cast<int8>(SRV_REGISTER::t0) + i);
	//	GEngine->GetComputeDescHeap()->SetSRV(mTextures[i]->GetSRVHandle(), reg);
	//}

	//// 파이프라인 세팅
	//mShader->Update();
}

void Material::Dispatch(uint32 x, uint32 y, uint32 z)
{
	// CBV + SRV + SetPipelineState
	//PushComputeData();

	// SetDescriptorHeaps + SetComputeRootDescriptorTable
	//GEngine->GetComputeDescHeap()->CommitTable();

	//COMPUTE_CMD_LIST->Dispatch(x, y, z);

	//GEngine->GetComputeCmdQueue()->FlushComputeCommandQueue();
}

shared_ptr<Material> Material::Clone()
{
	shared_ptr<Material> material = make_shared<Material>();

	material->SetShader(mShader);
	material->mParams = mParams;
	material->mTextures = mTextures;

	return material;
}

void Material::SetShader(std::wstring shaderID)
{

	shared_ptr<Shader> shader = RESOURCEMANAGER.Get<Shader>(shaderID);
	mShaderID = shaderID;
	


}
