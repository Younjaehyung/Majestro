#pragma once


class ConstantBuffer {
public:
	ConstantBuffer();

	~ConstantBuffer();

	void PushComputeData(void* buffer, uint32 size);
	//글로벌로 설정되어 한번만 작동하는 함수

	void PushData(void* buffer, uint32 size);	// TableDescriptor용


	void CreateBuffer(uint32 size);
	void CreateView(uint32 frameCount, uint32 startIndex, uint32 type, uint32 groupCount=0);
private:

	ComPtr<ID3D12Resource>	mCbvBuffer;	//GPU버퍼
	BYTE*					mMappedBuffer = nullptr;	//cpu쪽과 메모리 연결을 위한 포인터
	uint32					mElementSize = 0;	//buffer의 Size

	
	D3D12_CPU_DESCRIPTOR_HANDLE			mCpuHandleBegin = {};	//시작 DESCRIPTOR테이블 핸들
	uint32								mHandleIncrementSize = 0;	//한 DESCRIPTOR테이블당 크기

	uint32					mGroupIndex = {10};
	uint32					mStartIndex = {};
	uint32					mConstantIndex = {};			// CONSTANT Type
	uint32					mFrameCount = 0;
};


class StructuredBuffer
{
public:
	StructuredBuffer();
	~StructuredBuffer();


public:

	void PushGraphicsData(void* buffer, uint32 size);	// upload / default memcpy
	void PushDefaultToData(void* buffer, uint32 size);	// default to upload memcpy (dummy);

	void PushComputeSRVData(void* buffer, uint32 size);	// 추후 수정
	void PushComputeUAVData(void* buffer, uint32 size);

	// CPU 애니메이션용: 더미 업로드 버퍼에 데이터 쓰고 그래픽스 cmd list로 default로 카피
	void UpdateDefaultFromCpu(const void* data, uint32 size);

public:

	void SetResourceState(D3D12_RESOURCE_STATES state) { mResourceState = state; }
	D3D12_RESOURCE_STATES GetResourceState() { return mResourceState; }
	ComPtr<ID3D12Resource> GetBuffer() { return mBuffer; }
	uint64 GetBufferSize() const { return mBufferSize; }
	uint32 GetElementCount() const { return mElementCount; }
public:
	void CreateUploadBuffer(uint32 elementSize, uint32 elementCount, const wchar_t* debugName = L"StructuredUpload");
	void CreateDefaultBuffer(uint32 elementSize, uint32 elementCount, const wchar_t* debugName = L"StructuredDefault");


	void CreateSrvView(uint32 frameCount, uint32 startIndex ,uint32 type, uint32 groupCount=0);
	void CreateSrvViewAtIndex(uint32 descriptorIndex);

	void CreateUavView(uint32 frameCount, uint32 startIndex, uint32 type, uint32 groupCount=0);
	void CreateUavViewAtIndex(uint32 descriptorIndex);
private:
	bool EnsureDummyUploadBuffer(uint64 size);

	ComPtr<ID3D12Resource>			mBuffer;		// upload / default buffer
	ComPtr<ID3D12Resource>			mDummyBuffer;	// default to upload buffer (dummy);

	BYTE*							mMappedBuffer	= nullptr;	//cpu쪽과 메모리 연결을 위한 포인터
	BYTE*							mDummyMappedBuffer = nullptr;	// 더미 업로드 버퍼의 매핑 (지연 매핑)

	uint32						mElementSize = 0;	// 원소 하나 크기
	uint32						mElementCount = 0;	// 전체 원소 개수

	uint64						mBufferSize = 0;
	uint64						mDummyBufferSize = 0;
	std::wstring				mDebugName;

	D3D12_RESOURCE_STATES		mResourceState = D3D12_RESOURCE_STATE_COMMON;

	uint32						mGroupIndex = {};
	uint32						mStartIndex = {};
	uint32						mSrvIndex = {}; // Sructured Type
	uint32						mUavIndex = {}; // Sructured Type
	uint32						mFrameCount = 0;

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
