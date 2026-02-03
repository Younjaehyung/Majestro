#include "pch.h"
#include "Buffer.h"
#include "Engine.h"
#include "RenderManager.h"
#include "Texture.h"

ConstantBuffer::ConstantBuffer()
{

}


ConstantBuffer::~ConstantBuffer()
{
	cout << "delete buffer" << endl;

	if (mCbvBuffer)
	{
		if (mCbvBuffer != nullptr)
			mCbvBuffer->Unmap(0, nullptr);

		mCbvBuffer = nullptr;
	}
}

void ConstantBuffer::CreateBuffer(uint32 size)
{
	mElementSize = (size + 255) & ~255;

	//GPU에서 사용할 버퍼 생성
	uint32 bufferSize = mElementSize;
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	//버퍼 타입(UPLOAD) : CPU에서 VRAM으로 접근가능 메모리
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);	//버퍼 크기(생성)

	//VRAM(Upload)에 버퍼 생성
	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mCbvBuffer));

	//메모리 <=> 버퍼 연결
	//데이터를 넘길 수 있게 Mapping 해둠
	mCbvBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedBuffer));
	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).
}

void ConstantBuffer::CreateView(uint32 frameCount, uint32 startIndex, uint32 type, uint32 groupCount)
{

	assert(mElementSize != 0);

	mConstantIndex = type;
	mFrameCount = frameCount;
	mStartIndex = startIndex;
	mGroupIndex = groupCount;

	D3D12_CPU_DESCRIPTOR_HANDLE mHeapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	
	mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격
	mCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, ((static_cast<uint32>(mFrameCount) * groupCount) + mConstantIndex + startIndex) * mHandleIncrementSize);
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc {};
		cbvDesc.BufferLocation = mCbvBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = mElementSize;   // CB size is required to be 256-byte aligned.

	DEVICE->CreateConstantBufferView(&cbvDesc, mCpuHandleBegin);
	
}


void ConstantBuffer::PushComputeData(void* buffer, uint32 size)
{
	//assert(mCurrentIndex < mElementCount);
	//assert(mElementSize == ((size + 255) & ~255));

	//::memcpy(&mMappedBuffer[mCurrentIndex * mElementSize], buffer, size);

	//D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(mCurrentIndex);
	//gEngine->GetRenderManager()GetComputeDescHeap()->SetCBV(cpuHandle, mReg);

	//mCurrentIndex++;
}


void ConstantBuffer::PushData(void* buffer, uint32 size)
{

	assert(mElementSize == ((size + 255) & ~255));	//디버깅 코드(엉뚱한 데이터 확인용)

	::memcpy(mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
	// CPU → Upload Heap

}


//////////////////////////////////////////////////////////////////////////////////////

StructuredBuffer::StructuredBuffer()
{
}

StructuredBuffer::~StructuredBuffer()
{
}



void StructuredBuffer::PushGraphicsData(void* buffer, uint32 size)
{
	::memcpy(mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
}

void StructuredBuffer::PushDefaultToData(void* buffer, uint32 size)
{
	ComPtr<ID3D12Resource> readBuffer = nullptr;
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_NONE);
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	DEVICE->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&readBuffer));


	uint8* dataBegin = nullptr;
	D3D12_RANGE readRange{ 0, 0 };
	readBuffer->Map(0, &readRange, reinterpret_cast<void**>(&dataBegin));
	memcpy(dataBegin, buffer, size);

	// Common -> Copy
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		RESOURCE_CMD_LIST->ResourceBarrier(1, &barrier);
	}

	RESOURCE_CMD_LIST->CopyBufferRegion(mBuffer.Get(), 0, readBuffer.Get(), 0, size);

	// Copy -> Common
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		RESOURCE_CMD_LIST->ResourceBarrier(1, &barrier);
	}

	gEngine->GetRenderManager().GetGraphicsCmdQueue()->FlushResourceCommandQueue();

	mResourceState = D3D12_RESOURCE_STATE_COMMON;
}

void StructuredBuffer::PushComputeSRVData(void* buffer, uint32 size)
{
	::memcpy(mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
}

void StructuredBuffer::PushComputeUAVData(void* buffer, uint32 size)
{
	::memcpy(mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
}

void StructuredBuffer::CreateUploadBuffer(uint32 elementSize, uint32 elementCount)
{

	mElementSize = elementSize;		// 구조체 크기
	mElementCount = elementCount;	// StructuredBuffer에 들어갈 객체 개수
	// Buffer
	{
		uint64 bufferSize = static_cast<uint64>(mElementSize) * mElementCount;
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_NONE);
		D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		DEVICE->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mBuffer));
	}
	mBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedBuffer));
}

void StructuredBuffer::CreateDefaultBuffer(uint32 elementSize, uint32 elementCount)
{

	mElementSize = elementSize;		// 구조체 크기
	mElementCount = elementCount;	// StructuredBuffer에 들어갈 객체 개수

	// 추후 upload < = > default copy만들기
	// Buffer
	
		uint64 bufferSize = static_cast<uint64>(mElementSize) * mElementCount;
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		DEVICE->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			mResourceState,
			nullptr,
			IID_PPV_ARGS(&mBuffer));
	
	desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_NONE);

		// 2-1) Upload Heap 리소스 생성
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

		DEVICE->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mDummyBuffer));


}


void StructuredBuffer::CreateSrvView(uint32 frameCount, uint32 startIndex, uint32 type, uint32 groupCount)
{
	assert(mElementSize != 0);
	mGroupIndex = groupCount;
	mSrvIndex = type;
	mFrameCount = frameCount;
	mStartIndex = startIndex;

	D3D12_CPU_DESCRIPTOR_HANDLE mHeapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	uint32 mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격

	mSrvCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, ((static_cast<uint32>(mFrameCount) * groupCount) + type + startIndex) * mHandleIncrementSize);


	// 기본 
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mElementCount;
	srvDesc.Buffer.StructureByteStride = mElementSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	DEVICE->CreateShaderResourceView(mBuffer.Get(), &srvDesc, mSrvCpuHandleBegin);
}

void StructuredBuffer::CreateUavView(uint32 frameCount, uint32 startIndex, uint32 type, uint32 groupCount)
{
	assert(mElementSize!=0);

	mGroupIndex = groupCount;
	mUavIndex = type;
	mFrameCount = frameCount;
	mStartIndex = startIndex;

	D3D12_CPU_DESCRIPTOR_HANDLE mHeapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	uint32 mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격

	//mSrvCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, ((static_cast<uint32>(mFrameCount) * GROUP_COUNT) + static_cast<uint32>(type) + STRUCTURED_INDEX_START) * mHandleIncrementSize);
	mUavCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, ((static_cast<uint32>(mFrameCount) * mGroupIndex) + mUavIndex + mStartIndex) * mHandleIncrementSize);


	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = mElementCount;
	uavDesc.Buffer.StructureByteStride = mElementSize;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;


	DEVICE->CreateUnorderedAccessView(mBuffer.Get(), nullptr, &uavDesc, mUavCpuHandleBegin);


}
//////////////////////////////////////////////////////////////////////////////////

InstancingBuffer::InstancingBuffer()
{
}

InstancingBuffer::~InstancingBuffer()
{
}

void InstancingBuffer::Initialize(uint32 maxCount)
{
	mMaxCount = maxCount;

	const int32 bufferSize = sizeof(InstancingParams) * maxCount;
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mBuffer));
}

void InstancingBuffer::Clear()
{
	mData.clear();
}

void InstancingBuffer::AddData(InstancingParams& params)
{
	mData.push_back(params);
}

void InstancingBuffer::PushData()
{
	const uint32 dataCount = GetCount();
	if (dataCount > mMaxCount)
		Initialize(dataCount);

	const uint32 bufferSize = dataCount * sizeof(InstancingParams);

	void* dataBuffer = nullptr;
	D3D12_RANGE readRange{ 0, 0 };
	mBuffer->Map(0, &readRange, &dataBuffer);
	memcpy(dataBuffer, &mData[0], bufferSize);
	mBuffer->Unmap(0, nullptr);

	mBufferView.BufferLocation = mBuffer->GetGPUVirtualAddress();
	mBufferView.StrideInBytes = sizeof(InstancingParams);
	mBufferView.SizeInBytes = bufferSize;
}

