#pragma once


enum class CONSTANT_BUFFER_TYPE : uint8
{
	GLOBAL,	// Camera
	TRANSFORM,	// 삭제
	MATERIAL,
	
	END
};

enum class  STRUCTURED_BUFFER_TYPE : uint8	// 추후 추가 예정
{
	GLOBAL,
	LIGHT,
	TRANSFORM,
	MATERIAL,
	TEXTURE,
	BONE,
	PARTICLE,


	END
};

enum
{
	CONSTANT_BUFFER_COUNT = static_cast<uint8>(CONSTANT_BUFFER_TYPE::END),
	STRUCTURED_BUFFER_COUNT = static_cast<uint8>(STRUCTURED_BUFFER_TYPE::END)

};

class ConstantBuffer {
public:
	ConstantBuffer();
	~ConstantBuffer();

	void CreateConstantView(CONSTANT_INDEX type, uint32 size);					// RootDescriptor용

	void PushComputeData(void* buffer, uint32 size);
	//글로벌로 설정되어 한번만 작동하는 함수


	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32 index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32 index);

	void PushData(void* buffer, uint32 size);	// TableDescriptor용

private:
	void CreateBuffer();
	void CreateView(CONSTANT_INDEX);
private:

	ComPtr<ID3D12Resource>	mCbvBuffer;	//GPU버퍼
	BYTE*					mMappedBuffer = nullptr;	//cpu쪽과 메모리 연결을 위한 포인터
	uint32					mElementSize = 0;	//모든buffer의 Size
	uint32					mElementCount = 0;	//모든buffer의 카운터


	D3D12_CPU_DESCRIPTOR_HANDLE			mCpuHandleBegin = {};	//시작 DESCRIPTOR테이블 핸들
	uint32								mHandleIncrementSize = 0;	//한 DESCRIPTOR테이블당 크기

	CONSTANT_INDEX			mRootParmetersIndex = {};			// RootParmetersIndex번호
};

class ShaderResourceBuffer {
public:

};

class StructuredBuffer
{
public:
	StructuredBuffer();
	~StructuredBuffer();

	void Initialize(uint32 elementSize, uint32 elementCount);

	void PushGraphicsData(SRV_REGISTER reg);
	void PushComputeSRVData(SRV_REGISTER reg);
	void PushComputeUAVData(UAV_REGISTER reg);

	ComPtr<ID3D12DescriptorHeap> GetSRV() { return mCbvSrvHeap; }
	ComPtr<ID3D12DescriptorHeap> GetUAV() { return mUavHeap; }

	void SetResourceState(D3D12_RESOURCE_STATES state) { mResourceState = state; }
	D3D12_RESOURCE_STATES GetResourceState() { return mResourceState; }
	ComPtr<ID3D12Resource> GetBuffer() { return mBuffer; }

private:
	ComPtr<ID3D12Resource>			mBuffer;
	ComPtr<ID3D12DescriptorHeap>	mCbvSrvHeap;
	ComPtr<ID3D12DescriptorHeap>	mUavHeap;

	uint32						mElementSize = 0;
	uint32						mElementCount = 0;
	D3D12_RESOURCE_STATES		mResourceState = {};

private:
	D3D12_CPU_DESCRIPTOR_HANDLE mSrvHeapBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE mUavHeapBegin = {};
};



struct InstancingParams
{
	Matrix matWorld;
	Matrix matWV;
	Matrix matWVP;
};

//한 종류의 물건에는 한개의 instanceBuffer가 필요

class InstancingBuffer
{
public:
	InstancingBuffer();
	~InstancingBuffer();

	void Initialize(uint32 maxCount = 10);

	void Clear();
	void AddData(InstancingParams& params);
	void PushData();

public:
	uint32						GetCount() { return static_cast<uint32>(mData.size()); }
	ComPtr<ID3D12Resource>		GetBuffer() { return mBuffer; }
	D3D12_VERTEX_BUFFER_VIEW	GetBufferView() { return mBufferView; }

	void	SetID(uint64 instanceId) { mInstanceId = instanceId; }
	uint64	GetID() { return mInstanceId; }

private:
	uint64						mInstanceId = 0;
	ComPtr<ID3D12Resource>		mBuffer;
	D3D12_VERTEX_BUFFER_VIEW	mBufferView;

	uint32						mMaxCount = 0;
	vector<InstancingParams>	mData;
};