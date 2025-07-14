#pragma once
#include <vector>

// builder 패턴
class RootSignature
{
public:
	RootSignature() {};

	uint32 AddCBV(uint32 registerNum, uint32 space=0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	uint32 AddSRV(uint32 registerNum, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	uint32 AddUAV(uint32 registerNum, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	uint32 AddConstant(uint32 registerNum,uint32 values ,uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	uint32 AddTable(const std::vector<CD3DX12_DESCRIPTOR_RANGE>& ranges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);


	void AddSampler(const CD3DX12_STATIC_SAMPLER_DESC& sampler);
	void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature();	//루트시그니쳐 생성
	void CreateGraphicsRootSignature(uint8 num);	//루트시그니쳐 생성

	ComPtr<ID3D12RootSignature>	GetGraphicsRootSignature() { return mGraphicsRootSignature; }



	int GetParametersIndex() { return static_cast<int>(mRootParameters.size()) - 1; }

private:
	ComPtr<ID3D12RootSignature>	mGraphicsRootSignature;
	//ComPtr<ID3D12RootSignature>	_computeRootSignature;
	D3D12_STATIC_SAMPLER_DESC mSamplerDesc;	//샘플러용

	std::vector<CD3DX12_ROOT_PARAMETER> mRootParameters;
	std::vector<CD3DX12_DESCRIPTOR_RANGE> mRanges; // 평탄화된 ranges
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> mSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS mFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
private:
	


};

