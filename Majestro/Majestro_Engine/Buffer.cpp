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
	if (mCbvBuffer)
	{
		if (mCbvBuffer != nullptr)
			mCbvBuffer->Unmap(0, nullptr);

		mCbvBuffer = nullptr;
	}
}

void ConstantBuffer::CreateConstantView(CONSTANT_INDEX type,uint32 size)
{
	mRootParmetersIndex =type;

	// 상수 버퍼는 256 바이트 배수로 만들어야 한다
	// 0 256 512 768
	mElementSize = (size + 255) & ~255;


	CreateBuffer();
	CreateView(type);
}

void ConstantBuffer::CreateBuffer()
{
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

void ConstantBuffer::CreateView(CONSTANT_INDEX type)
{
	D3D12_CPU_DESCRIPTOR_HANDLE mHeapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	
	mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격
	mCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, static_cast<uint32>(type) * mHandleIncrementSize);
	
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


void ConstantBuffer::PushData(void* buffer, uint32 size)
{

	assert(mElementSize == ((size + 255) & ~255));	//디버깅 코드(엉뚱한 데이터 확인용)


	::memcpy(&mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
	// CPU → Upload Heap

}


//////////////////////////////////////////////////////////////////////////////////////

StructuredBuffer::StructuredBuffer()
{
}

StructuredBuffer::~StructuredBuffer()
{
}

void StructuredBuffer::CreateStructuredView(STRUCTURED_INDEX type, uint32 elementSize, uint32 elementCount)
{
	mRootParmetersIndex = type;

	mElementSize = elementSize;		// 구조체 크기
	mElementCount = elementCount;	// StructuredBuffer에 들어갈 객체 개수
	mResourceState = D3D12_RESOURCE_STATE_COMMON;
	
	//GPU에 원하는 버퍼 생성

	CreateBuffer();
	CreateView(type);

	
}

void StructuredBuffer::PushGraphicsData(void* buffer, uint32 size)
{
	::memcpy(&mMappedBuffer, buffer, size);	//버퍼에 데이터 전달(복사(즉시))
}

void StructuredBuffer::PushComputeSRVData(SRV_REGISTER reg)
{
	//gEngine->GetComputeDescHeap()->SetSRV(_srvHeapBegin, reg);
}

void StructuredBuffer::PushComputeUAVData(UAV_REGISTER reg)
{
	//gEngine->GetComputeDescHeap()->SetUAV(mUavHeapBegin, reg);
}

void StructuredBuffer::CreateBuffer()
{

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
	mBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedBuffer));
}

void StructuredBuffer::CreateView(STRUCTURED_INDEX type)
{

	D3D12_CPU_DESCRIPTOR_HANDLE mHeapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	uint32 mHandleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격

	mSrvCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, static_cast<uint32>(type) * mHandleIncrementSize);

	switch (type) {
	case STRUCTURED_INDEX::SRV_PARTICLE_INDEX:	// Particle일시 UAV용도 생성
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = mElementCount;
		uavDesc.Buffer.StructureByteStride = mElementSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		mUavCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeapHandleBegin, static_cast<uint64>(STRUCTURED_INDEX::UAV_PARTICLE_INDEX) * mHandleIncrementSize);

		DEVICE->CreateUnorderedAccessView(mBuffer.Get(), nullptr, &uavDesc, mSrvCpuHandleBegin);
		break;
	}

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
//////////////////////////////////////////////////////////////////////////////////



TextureBuffer::TextureBuffer()
{
}

TextureBuffer::~TextureBuffer()
{
}

void TextureBuffer::CreateTextureBuffer(shared_ptr<Texture> texture, D3D12_SRV_DIMENSION viewDimension)
{

	UINT mipLevels = 1;	// 임시 밈맵

	// Texture에 대한 SRV 서술자 설정
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 기본 매핑 (RGBA -> RGBA)
	srvDesc.Format = texture->GetOriginalImage().GetMetadata().format; // 텍스처의 실제 포맷

	// ViewDimension에 따라 다른 구조체 필드를 설정
	srvDesc.ViewDimension = viewDimension;

	switch (viewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE2D:
		srvDesc.Texture2D.MostDetailedMip = 0;       // 가장 높은 해상도의 밉맵부터 시작
		srvDesc.Texture2D.MipLevels = mipLevels;     // 사용할 밉맵 레벨 수
		srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스 (비디오 텍스처 등에서 사용)
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 최소 LOD 클램프
		break;
	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = texture->GetTex2D()->GetDesc().DepthOrArraySize; // 배열 크기
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		break;
	case D3D12_SRV_DIMENSION_TEXTURECUBE:
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
		// 다른 텍스처 차원에 대한 case 추가 (3D, CubeArray 등)
	default:
		// 지원하지 않는 차원에 대한 오류 처리
		break;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE heapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	uint32& lastIndex = RENDERMANAGER.GetGraphicsDescHeap()->GetLastIndex();

	uint32 handleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//핸들간 간격

	mCpuHandleBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapHandleBegin, lastIndex * handleIncrementSize);
	lastIndex++;

	// SRV 생성
	DEVICE->CreateShaderResourceView(texture->GetTex2D().Get(), &srvDesc, mCpuHandleBegin);
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

