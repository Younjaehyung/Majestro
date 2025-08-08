#pragma once


class ConstantBuffer {
public:
	ConstantBuffer();

	~ConstantBuffer();

	void PushComputeData(void* buffer, uint32 size);
	//글로벌로 설정되어 한번만 작동하는 함수

	void PushData(void* buffer, uint32 size);	// TableDescriptor용


	void CreateBuffer(uint32 size);
	void CreateView(uint8 frameCount, CONSTANT_INDEX type);
private:

	ComPtr<ID3D12Resource>	mCbvBuffer;	//GPU버퍼
	BYTE*					mMappedBuffer = nullptr;	//cpu쪽과 메모리 연결을 위한 포인터
	uint32					mElementSize = 0;	//buffer의 Size

	
	D3D12_CPU_DESCRIPTOR_HANDLE			mCpuHandleBegin = {};	//시작 DESCRIPTOR테이블 핸들
	uint32								mHandleIncrementSize = 0;	//한 DESCRIPTOR테이블당 크기

	CONSTANT_INDEX			mConstantIndex = {};			// CONSTANT Type
	uint8					mFrameCount = 0;
};


class StructuredBuffer
{
public:
	StructuredBuffer();
	~StructuredBuffer();




	void PushGraphicsData(void* buffer, uint32 size);
	void PushComputeSRVData(void* buffer, uint32 size);	// 추후 수정
	void PushComputeUAVData(void* buffer, uint32 size);



	void SetResourceState(D3D12_RESOURCE_STATES state) { mResourceState = state; }
	D3D12_RESOURCE_STATES GetResourceState() { return mResourceState; }
	ComPtr<ID3D12Resource> GetBuffer() { return mBuffer; }

	void CreateUploadBuffer(uint32 elementSize, uint32 elementCount);
	void CreateDefaultBuffer(uint32 elementSize, uint32 elementCount);

	void CreateSrvView(uint8 frameCount, STRUCTURED_INDEX type);
	void CreateUavView(uint8 frameCount, PARTICLE_INDEX type);
private:
	ComPtr<ID3D12Resource>			mBuffer;
	BYTE*							mMappedBuffer	= nullptr;	//cpu쪽과 메모리 연결을 위한 포인터

	uint32						mElementSize = 0;
	uint32						mElementCount = 0;

	D3D12_RESOURCE_STATES		mResourceState = D3D12_RESOURCE_STATE_COMMON;

	STRUCTURED_INDEX			mSrvIndex = {}; // Sructured Type
	PARTICLE_INDEX				mUavIndex = {}; // Sructured Type
	uint8						mFrameCount = 0;
private:
	D3D12_CPU_DESCRIPTOR_HANDLE mSrvCpuHandleBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE mUavCpuHandleBegin = {};

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