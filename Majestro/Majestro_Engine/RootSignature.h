#pragma once

// builder 패턴
class RootSignature
{
public:
	void Initialize();

	void AddCBV();
	void AddSRV();
	void AddUAV();
	void AddConstant();
	void AddTable();
	ComPtr<ID3D12RootSignature>	GetGraphicsRootSignature() { return mGraphicsRootSignature; }
	//ComPtr<ID3D12RootSignature>	GetComputeRootSignature() { return _computeRootSignature; }

private:
	ComPtr<ID3D12RootSignature>	mGraphicsRootSignature;
	//ComPtr<ID3D12RootSignature>	_computeRootSignature;
	D3D12_STATIC_SAMPLER_DESC mSamplerDesc;	//샘플러용

	uint8 mParametersIndex;

private:
	//void CreateComputeRootSignature();
	void CreateGraphicsRootSignature();	//루트시그니쳐 생성
	void CreateGraphicsRootSignature(uint8 num);	//루트시그니쳐 생성

};

