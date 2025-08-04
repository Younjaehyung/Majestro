#include "pch.h"
#include "Texture.h"
#include "Engine.h"
#include "RenderManager.h"

Texture::Texture() : Object(OBJECT_TYPE::TEXTURE)
{
}

Texture::~Texture()
{
}

void Texture::Load(const wstring& path)
{
	// ���� Ȯ���� ���
	wstring ext = std::filesystem::path(path).extension();

	if (ext == L".dds" || ext == L".DDS")
		::LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, mOriginalImage);
	else if (ext == L".tga" || ext == L".TGA")
		::LoadFromTGAFile(path.c_str(), nullptr, mOriginalImage);
	else // png, jpg, jpeg, bmp
		::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, mOriginalImage);

	HRESULT hr = ::CreateTexture(DEVICE.Get(), mOriginalImage.GetMetadata(), &mImage);
	if (FAILED(hr))
		assert(nullptr);

	vector<D3D12_SUBRESOURCE_DATA> subResources;

	mDescription = mImage->GetDesc();

	hr = ::PrepareUpload(DEVICE.Get(),
		mOriginalImage.GetImages(),
		mOriginalImage.GetImageCount(),
		mOriginalImage.GetMetadata(),
		subResources);

	if (FAILED(hr))
		assert(nullptr);

	const uint64 bufferSize = ::GetRequiredIntermediateSize(mImage.Get(), 0, static_cast<uint32>(subResources.size()));

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	ComPtr<ID3D12Resource> textureUploadHeap;
	hr = DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(textureUploadHeap.GetAddressOf()));

	if (FAILED(hr))
		assert(nullptr);

	::UpdateSubresources(RESOURCE_CMD_LIST.Get(),
		mImage.Get(),
		textureUploadHeap.Get(),
		0, 0,
		static_cast<unsigned int>(subResources.size()),
		subResources.data());

	gEngine->GetRenderManager().GetGraphicsCmdQueue()->FlushResourceCommandQueue();

	CreateFromResource(mImage);

	//D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	//srvHeapDesc.NumDescriptors = 1;
	//srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//DEVICE->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap));

	//mSrvHeapBegin = mSrvHeap->GetCPUDescriptorHandleForHeapStart();

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = mOriginalImage.GetMetadata().format;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.Texture2D.MipLevels = 1;
	//DEVICE->CreateShaderResourceView(mImage.Get(), &srvDesc, mSrvHeapBegin);
}

void Texture::Create(DXGI_FORMAT format, uint32 width, uint32 height,
	const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
	D3D12_RESOURCE_FLAGS resFlags, Vec4 clearColor)
{
	mDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
	mDescription.Flags = resFlags;
	

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;

	D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

	if (resFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		resourceStates = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		optimizedClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
		pOptimizedClearValue = &optimizedClearValue;
	}
	else if (resFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		resourceStates = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		float arrFloat[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
		optimizedClearValue = CD3DX12_CLEAR_VALUE(format, arrFloat);
		pOptimizedClearValue = &optimizedClearValue;
	}

	// Create Texture2D
	HRESULT hr = DEVICE->CreateCommittedResource(
		&heapProperty,
		heapFlags,
		&mDescription,
		resourceStates,
		pOptimizedClearValue,
		IID_PPV_ARGS(&mImage));

	assert(SUCCEEDED(hr));


	
	CreateFromResource(mImage);
	
	
}

void Texture::CreateFromResource(ComPtr<ID3D12Resource> tex2D)
{
	mImage = tex2D;

	mDescription = tex2D->GetDesc();


	if (mDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ||
		mDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		return;
	}

	UINT mipLevels = 1;	// 임시 밈맵

	// Texture에 대한 SRV 서술자 설정
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 기본 매핑 (RGBA -> RGBA)
	srvDesc.Format = mOriginalImage.GetMetadata().format; // 텍스처의 실제 포맷

	// ViewDimension에 따라 다른 구조체 필드를 설정
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	switch (srvDesc.ViewDimension)
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
		srvDesc.Texture2DArray.ArraySize = mImage->GetDesc().DepthOrArraySize; // 배열 크기
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

	mSrvHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapHandleBegin, lastIndex * handleIncrementSize);
	mSrvIndex = lastIndex;
	lastIndex++;

	// SRV 생성
	DEVICE->CreateShaderResourceView(mImage.Get(), &srvDesc, mSrvHeapBegin);


	// UAV
	if (mDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		mUavHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapHandleBegin, lastIndex * handleIncrementSize);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = mOriginalImage.GetMetadata().format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		DEVICE->CreateUnorderedAccessView(mImage.Get(), nullptr, &uavDesc, mUavHeapBegin);

		mUavIndex = lastIndex;
		lastIndex++;
	}

	
}