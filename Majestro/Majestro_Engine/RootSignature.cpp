#include "pch.h"
#include "RootSignature.h"
#include "Engine.h"
#include "RenderManager.h"


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

	
	hr = DEVICE->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mGraphicsRootSignature));
	if (FAILED(hr)) return nullptr;


	return mGraphicsRootSignature;
}

void RootSignature::CreateGraphicsRootSignature(uint8 num)
{
	mSamplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);//샘플러 생성


	CD3DX12_DESCRIPTOR_RANGE ranges[] =
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_REGISTER_COUNT - 1, 1), // b1~b4 몇번부터 몇개까지 레지스터를 사용할건지 작성
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_REGISTER_COUNT, 0), // t0~t4 몇번부터 몇개까지 레지스터를 사용할건지 작성(리소스)
	};

	CD3DX12_ROOT_PARAMETER param[2];	//DescriptorTable 생성
	param[0].InitAsConstantBufferView(static_cast<uint32>(CBV_REGISTER::b0));	//b0 레지스터 전역으로 활용
	param[1].InitAsDescriptorTable(_countof(ranges), ranges);	//DescriptorTable 크기 지정


	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param, 1, &mSamplerDesc);	//샘플러
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 입력 조립기 단계

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;
	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	DEVICE->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&mGraphicsRootSignature));
}


