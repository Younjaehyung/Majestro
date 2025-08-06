#pragma once


class GraphicsDescriptorHeap
{
public:
	void Initialize(uint32);
	void Clear();


	void CommitTable(uint8 type);
	void CommitTexutreTable();

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return mDescHeap; }


	uint32& GetLastIndex() { return mLastIndex; }
private:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint8 reg);

private:

	ComPtr<ID3D12DescriptorHeap> mDescHeap;
	uint64					mHandleSize = 0;	//핸들 사이즈
	uint64					mGroupSize = 0;	//힙 그룹별(핸들 * 그룹내 갯수) 사이즈
	uint32					mGroupCount = 0; //힙 그룹별 카운터
	
	
	uint32					mTextureGroupIndex = static_cast<uint32>(TEXTURE_INDEX::TEXTURE_INDEX); //힙 Texture 시작 index
	uint32					mLastIndex = static_cast<uint32>(TEXTURE_INDEX_START); //힙 Texture 마지막 index
	uint32					mCurrentGroupIndex = 0;
};

