#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize()
{

}

void RootSignature::CreateGraphicsRootSignature()
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


