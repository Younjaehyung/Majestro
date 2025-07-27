#include "pch.h"
#include "RootSignature.h"
#include "Engine.h"
#include "RenderManager.h"


RootSignature::RootSignature() : Object(OBJECT_TYPE::ROOTSIGNATURE)
{
}

uint32 RootSignature::AddCBV(uint32 registerNum, uint32 space, D3D12_SHADER_VISIBILITY visibility)
{
	CD3DX12_ROOT_PARAMETER param;
	param.InitAsConstantBufferView(registerNum, space, visibility);
	mRootParameters.push_back(param);
	return static_cast<uint32>(mRootParameters.size() - 1);
}

uint32 RootSignature::AddSRV(uint32 registerNum, uint32 space, D3D12_SHADER_VISIBILITY visibility)
{
	CD3DX12_ROOT_PARAMETER param;
	param.InitAsShaderResourceView(registerNum, space, visibility);
	mRootParameters.push_back(param);
	return static_cast<uint32>(mRootParameters.size() - 1);
}

uint32 RootSignature::AddUAV(uint32 registerNum, uint32 space, D3D12_SHADER_VISIBILITY visibility)
{
	CD3DX12_ROOT_PARAMETER param;
	param.InitAsUnorderedAccessView(registerNum, space, visibility);
	mRootParameters.push_back(param);
	return static_cast<uint32>(mRootParameters.size() - 1);
}

uint32 RootSignature::AddConstant(uint32 registerNum, uint32 values,uint32 space, D3D12_SHADER_VISIBILITY visibility)
{
	CD3DX12_ROOT_PARAMETER param;
	param.InitAsConstants(values, registerNum, space, visibility);
	mRootParameters.push_back(param);
	return static_cast<uint32_t>(mRootParameters.size() - 1);
}

uint32 RootSignature::AddTable(const std::vector<CD3DX12_DESCRIPTOR_RANGE>& ranges,D3D12_SHADER_VISIBILITY visibility)
{
	size_t offset = mRanges.size();
	mRanges.insert(mRanges.end(), ranges.begin(), ranges.end());

	CD3DX12_ROOT_PARAMETER param;
	param.InitAsDescriptorTable(static_cast<UINT>(ranges.size()), &mRanges[offset], visibility);
	mRootParameters.push_back(param);

	return static_cast<uint32_t>(mRootParameters.size() - 1);
}



void RootSignature::AddSampler(const CD3DX12_STATIC_SAMPLER_DESC& sampler)
{
	mSamplers.push_back(sampler);
}

void RootSignature::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	// D3D12_ROOT_SIGNATURE_FLAG_NONE : Compute Shader 전용
	mFlags = flags;
}




ComPtr<ID3D12RootSignature> RootSignature::CreateGraphicsRootSignature()
{
	CD3DX12_ROOT_SIGNATURE_DESC desc(
		static_cast<UINT>(mRootParameters.size()),
		mRootParameters.data(),
		static_cast<UINT>(mSamplers.size()),
		mSamplers.data(),
		mFlags
	);

	ComPtr<ID3DBlob> sigBlob;
	ComPtr<ID3DBlob> errBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) ::OutputDebugStringA((char*)errBlob->GetBufferPointer());
		return nullptr;
	}

	
	hr = DEVICE->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
	if (FAILED(hr)) return nullptr;


	return mRootSignature;
}

ComPtr<ID3D12RootSignature> RootSignature::CreateComputeRootSignature()
{
	CD3DX12_ROOT_SIGNATURE_DESC desc(
		static_cast<UINT>(mRootParameters.size()),
		mRootParameters.data(),
		static_cast<UINT>(mSamplers.size()),
		mSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_NONE
	);

	ComPtr<ID3DBlob> sigBlob;
	ComPtr<ID3DBlob> errBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) ::OutputDebugStringA((char*)errBlob->GetBufferPointer());
		return nullptr;
	}


	hr = DEVICE->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
	if (FAILED(hr)) return nullptr;


	
	return mRootSignature;
}


