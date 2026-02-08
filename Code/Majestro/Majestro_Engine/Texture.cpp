#include "pch.h"
#include "Texture.h"
#include "Engine.h"
#include "RenderManager.h"

uint32 Texture::mTextureCount = 0;
uint32 Texture::mCubeMapCount = 0;

Texture::Texture() : Object(OBJECT_TYPE::TEXTURE)
{

}

Texture::~Texture()
{
}

void Texture::Load(const wstring& path)
{

	wstring ext = std::filesystem::path(path).extension();
	mIsCubeMap = false;
	if (ext == L".dds" || ext == L".DDS")
		::LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, mOriginalImage);
	else if (ext == L".tga" || ext == L".TGA")
		::LoadFromTGAFile(path.c_str(), nullptr, mOriginalImage);
	else // png, jpg, jpeg, bmp
		::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, mOriginalImage);

	HRESULT hr = ::CreateTexture(DEVICE.Get(), mOriginalImage.GetMetadata(), &mImage);
	if (FAILED(hr))
		assert(nullptr);
	mIsCubeMap = mOriginalImage.GetMetadata().IsCubemap();
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
	D3D12_RESOURCE_FLAGS resFlags, bool createSRVUAV, int massCount,int msaaQality, Vec4 clearColor)
{
	mDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height,1, 1, massCount, msaaQality);
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

	mIsMSAA = (massCount > 1) ? true : false;
	mMSAACount = massCount;

	CreateFromResource(mImage, createSRVUAV, mMSAACount);


}

void Texture::CreateFromResource(ComPtr<ID3D12Resource> tex2D, bool createSRVUAV, int isMSAA,TextureType use)
{
	mImage = tex2D;

	mDescription = tex2D->GetDesc();

	if (!createSRVUAV) {
		return;
	}

	mMipLevels = mOriginalImage.GetMetadata().mipLevels;	// 임시 밈맵
	
	if (mMipLevels == 0)
		mMipLevels = mDescription.MipLevels;



	// Texture에 대한 SRV 서술자 설정
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 기본 매핑 (RGBA -> RGBA)
	srvDesc.Format = mDescription.Format;



	if (mIsCubeMap && use == TextureType::TEXTURE_2D) {
		use = TextureType::TEXTURE_CUBE_ARRAY;
	}
	else if(isMSAA > 1 && use == TextureType::TEXTURE_2D)
	{
		use = TextureType::TEXTURE_MSAA;
	}

	switch (use)
	{
	case TextureType::TEXTURE_2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;       // 가장 높은 해상도의 밉맵부터 시작
		srvDesc.Texture2D.MipLevels = mMipLevels;     // 사용할 밉맵 레벨 수
		srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스 (비디오 텍스처 등에서 사용)
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 최소 LOD 클램프
		break;
	case TextureType::TEXTURE_2D_ARRAY:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mMipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = mImage->GetDesc().DepthOrArraySize; // 배열 크기
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		break;
	case TextureType::TEXTURE_CUBE:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mMipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case TextureType::TEXTURE_CUBE_ARRAY:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.MostDetailedMip = 0;
		srvDesc.TextureCubeArray.MipLevels = mMipLevels;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;
		srvDesc.TextureCubeArray.NumCubes = mImage->GetDesc().DepthOrArraySize / 6; // 큐브 개수
		srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
		break;
	case TextureType::TEXTURE_MSAA:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		srvDesc.Texture2D.MostDetailedMip = 0;       // 가장 높은 해상도의 밉맵부터 시작
		srvDesc.Texture2D.MipLevels = mMipLevels;     // 사용할 밉맵 레벨 수
		srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스 (비디오 텍스처 등에서 사용)
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 최소 LOD 클램프
		break;
	case TextureType::TEXTURE_MSAA_ARRAY:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mMipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = mImage->GetDesc().DepthOrArraySize; // 배열 크기
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		break;


	default:
		// 지원하지 않는 차원에 대한 오류 처리
		break;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE heapHandleBegin = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	uint32& lastIndex = RENDERMANAGER.GetGraphicsDescHeap()->GetLastIndex();
	uint32 handleIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (mIsCubeMap)
	{
		mCubeMapIndex = mCubeMapCount;
		const uint32 descriptorIndex = TEXTURE_CUBE_INDEX_START + mCubeMapIndex;
		mSrvHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapHandleBegin, descriptorIndex * handleIncrementSize);
		mSrvIndex = descriptorIndex;
		mCubeMapCount++;
		lastIndex = max(lastIndex, descriptorIndex + 1);
		mImageMapIndex = 0;
	}
	else
	{
		mImageMapIndex = mTextureCount;
		const uint32 descriptorIndex = TEXTURE_INDEX_START + mImageMapIndex;
		mSrvHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapHandleBegin, descriptorIndex * handleIncrementSize);
		mSrvIndex = descriptorIndex;
		mTextureCount++;
		lastIndex = max(lastIndex, descriptorIndex + 1);
		mCubeMapIndex = 0;
	}


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

float Texture::GetHeightValue(float u, float v) const
{
	if (mOriginalImage.GetImageCount() == 0)
		return 0.0f;

	const DirectX::Image* image = mOriginalImage.GetImage(0, 0, 0);
	if (!image || !image->pixels)
		return 0.0f;

	u = std::clamp(u, 0.0f, 1.0f);
	v = std::clamp(v, 0.0f, 1.0f);

	float fx = u * (image->width - 1);
	float fy = v * (image->height - 1);

	uint32 x0 = static_cast<uint32>(fx);
	uint32 y0 = static_cast<uint32>(fy);
	uint32 x1 = min(x0 + 1, static_cast<uint32>(image->width - 1));
	uint32 y1 = min(y0 + 1, static_cast<uint32>(image->height - 1));

	float tx = fx - x0;
	float ty = fy - y0;

	// 수정: bilinear 필터링을 위한 주변 4 픽셀 샘플
	float h00 = GetHeightValuePixel(x0, y0);
	float h10 = GetHeightValuePixel(x1, y0);
	float h01 = GetHeightValuePixel(x0, y1);
	float h11 = GetHeightValuePixel(x1, y1);

	float h0 = std::lerp(h00, h10, tx);
	float h1 = std::lerp(h01, h11, tx);
	return std::lerp(h0, h1, ty);
}
float Texture::GetHeightValuePixel(uint32 x, uint32 y) const
{
	if (mOriginalImage.GetImageCount() == 0)
		return 0.0f;

	const DirectX::Image* image = mOriginalImage.GetImage(0, 0, 0);
	if (!image || !image->pixels)
		return 0.0f;

	if (x >= image->width || y >= image->height)
		return 0.0f;

	// [수정] 포맷별 bytesPerPixel 계산
	size_t bytesPerPixel = 0;
	switch (image->format)
	{
	case DXGI_FORMAT_R8_UNORM:
		bytesPerPixel = 1;
		break;
	case DXGI_FORMAT_R16_UNORM:
		bytesPerPixel = 2;
		break;
	case DXGI_FORMAT_R32_FLOAT:
		bytesPerPixel = 4;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		bytesPerPixel = 4;
		break;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		bytesPerPixel = 16;
		break;
	default:
		bytesPerPixel = 4; // 최소 기본값
		break;
	}

	// [수정] rowPitch 와 bytesPerPixel 을 같이 사용
	const uint8_t* pixel =
		image->pixels + y * image->rowPitch + x * bytesPerPixel;

	float height = 0.0f;

	switch (image->format)
	{
	case DXGI_FORMAT_R8_UNORM:
		height = pixel[0] / 255.0f;
		break;
	case DXGI_FORMAT_R16_UNORM:
		height = (*reinterpret_cast<const uint16_t*>(pixel)) / 65535.0f;
		break;
	case DXGI_FORMAT_R32_FLOAT:
		height = *reinterpret_cast<const float*>(pixel);
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		height = pixel[0] / 255.0f;
		break;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		height = reinterpret_cast<const float*>(pixel)[0];
		break;
	default:
		height = pixel[0] / 255.0f;
		break;
	}

	return height;
}
