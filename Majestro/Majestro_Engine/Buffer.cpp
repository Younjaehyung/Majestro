#include "pch.h"
#include "Buffer.h"
#include "Engine.h"
#include "RenderManager.h"

ConstantBuffer::ConstantBuffer()
{
}

ConstantBuffer::~ConstantBuffer()
{
	if (mCbvBuffer)
	{
		if (mCbvBuffer != nullptr)
			mCbvBuffer->Unmap(0, nullptr);

		mCbvBuffer = nullptr;
	}
}



void ConstantBuffer::Initialize(CBV_REGISTER reg, uint32 size, uint32 count)
{
	mReg = reg;

	// 상수 버퍼는 256 바이트 배수로 만들어야 한다
	// 0 256 512 768
	mElementSize = (size + 255) & ~255;
	mElementCount = count;

	CreateBuffer();
	CreateView();
}

void ConstantBuffer::CreateBuffer()
{
	//GPU에서 사용할 버퍼 생성
	uint32 bufferSize = mElementSize * mElementCount;
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

void ConstantBuffer::CreateView()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvDesc = {};
	cbvDesc.NumDescriptors = mElementCount;
	cbvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DEVICE->CreateDescriptorHeap(&cbvDesc, IID_PPV_ARGS(&mCbvHeap));	//힙생성

	mCpuHandleBegin = mCbvHeap->GetCPUDescriptorHandleForHeapStart();	//시작핸들
	mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격

	for (uint32 i = 0; i < mElementCount; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = GetCpuHandle(i);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCbvBuffer->GetGPUVirtualAddress() + static_cast<uint64>(mElementSize) * i;
		cbvDesc.SizeInBytes = mElementSize;   // CB size is required to be 256-byte aligned.

		DEVICE->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}
}




void ConstantBuffer::Clear()
{
	mCurrentIndex = 0;
}

void ConstantBuffer::PushGraphicsData(void* buffer, uint32 size)
{
	assert(mCurrentIndex < mElementCount);	//디버깅 코드(데이터와 버퍼크기의 오버플로우 체크)
	assert(mElementSize == ((size + 255) & ~255));	//디버깅 코드(엉뚱한 데이터 확인용)


	::memcpy(&mMappedBuffer[mCurrentIndex * mElementSize], buffer, size);	//버퍼에 데이터 전달(복사(즉시))
	// CPU → Upload Heap

//D3D12_GPU_VIRTUAL_ADDRESS address = GetGpuVirtualAddress ( _currentIndex );	//CMD을 이용하여 레지스터에 버퍼주소값 전달
//CMD_LIST->SetGraphicsRootConstantBufferView ( rootParamIndex , address );	//rootSignature의 레지스터 번호 전달

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(mCurrentIndex);

	gEngine->GetRenderManager().GetGraphicsDescHeap()->SetCBV(cpuHandle, mReg);

	mCurrentIndex++;

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

void ConstantBuffer::SetGraphicsGlobalData(void* buffer, uint32 size)
{
	assert(mElementSize == ((size + 255) & ~255));
	::memcpy(&mMappedBuffer[0], buffer, size);	//b0에 데이터를 넣어라(데이터가 1개만 전역이니)
	GRAPHICS_CMD_LIST->SetGraphicsRootConstantBufferView(0, GetGpuVirtualAddress(0));
}



D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGpuVirtualAddress(uint32 index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mCbvBuffer->GetGPUVirtualAddress();
	objCBAddress += index * mElementSize;
	return objCBAddress;
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetCpuHandle(uint32 index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mCpuHandleBegin, index * mHandleIncrementSize);
}


//////////////////////////////////////////////////////////////////////////////////////

StructuredBuffer::StructuredBuffer()
{
}

StructuredBuffer::~StructuredBuffer()
{
}

void StructuredBuffer::Initialize(uint32 elementSize, uint32 elementCount)
{
	mElementSize = elementSize;
	mElementCount = elementCount;
	mResourceState = D3D12_RESOURCE_STATE_COMMON;

	//GPU에 원하는 버퍼 생성

	// Buffer
	{
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
	}

	// SRV
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DEVICE->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap));

		mSrvHeapBegin = mSrvHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = mElementCount;
		srvDesc.Buffer.StructureByteStride = mElementSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		DEVICE->CreateShaderResourceView(mBuffer.Get(), &srvDesc, mSrvHeapBegin);
	}

	// UAV
	{
		D3D12_DESCRIPTOR_HEAP_DESC uavheapDesc = {};
		uavheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		uavheapDesc.NumDescriptors = 1;
		uavheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		DEVICE->CreateDescriptorHeap(&uavheapDesc, IID_PPV_ARGS(&mUavHeap));

		mUavHeapBegin = mUavHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = mElementCount;
		uavDesc.Buffer.StructureByteStride = mElementSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		DEVICE->CreateUnorderedAccessView(mBuffer.Get(), nullptr, &uavDesc, mUavHeapBegin);
	}
}

void StructuredBuffer::PushGraphicsData(SRV_REGISTER reg)
{
	Graphics_DescHeap->SetSRV(mSrvHeapBegin, reg);
}

void StructuredBuffer::PushComputeSRVData(SRV_REGISTER reg)
{
	//gEngine->GetComputeDescHeap()->SetSRV(_srvHeapBegin, reg);
}

void StructuredBuffer::PushComputeUAVData(UAV_REGISTER reg)
{
	//gEngine->GetComputeDescHeap()->SetUAV(mUavHeapBegin, reg);
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