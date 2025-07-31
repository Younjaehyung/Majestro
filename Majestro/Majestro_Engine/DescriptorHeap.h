#pragma once


class GraphicsDescriptorHeap
{
public:
	void Initialize(uint32);
	void Clear();
	void SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg);
	void SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg);
	void CommitTable();

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return mDescHeap; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(CBV_REGISTER reg);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(SRV_REGISTER reg);

	uint32& GetLastIndex() { return mLastIndex; }
private:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint8 reg);

private:

	ComPtr<ID3D12DescriptorHeap> mDescHeap;
	uint64					mHandleSize = 0;	//핸들 사이즈
	uint64					mGroupSize = 0;	//힙 그룹별(핸들 * 그룹내 갯수) 사이즈
	uint64					mGroupCount = 0; //힙 그룹별 카운터
	
	
	uint32					mTextureGroupIndex = static_cast<uint32>(TEXTURE_INDEX::TEXTURE_INDEX); //힙 Texture 시작 index
	uint32					mLastIndex = 0; //힙 Texture 시작 index
	uint32					mCurrentGroupIndex = 0;
};

