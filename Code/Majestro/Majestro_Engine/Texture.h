#pragma once
#include "Object.h"


enum class TextureType : uint8
{
	TEXTURE_2D,
	TEXTURE_2D_ARRAY,
	TEXTURE_CUBE,
	TEXTURE_CUBE_ARRAY,
	TEXTURE_MSAA,
	TEXTURE_MSAA_ARRAY
};

class Texture : public Object
{
public:
	Texture();
	virtual ~Texture();

	virtual void Load(const wstring& path);	//리소스 이미지 로딩


	void Create(DXGI_FORMAT format, uint32 width, uint32 height,
		const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
		D3D12_RESOURCE_FLAGS resFlags, bool createSRVUAV = 1, int massCount = 1, int msaaQality = 1, Vec4 clearColor = Vec4(), uint16 arraySize = 1, TextureType type = TextureType::TEXTURE_2D);
	//새로운 사용자 텍스쳐 생성

	void CreateFromResource(ComPtr<ID3D12Resource> tex2D, bool createSRVUAV = 1, int isMSAA = 0, TextureType use = TextureType::TEXTURE_2D);
	//버퍼를 이용해서 텍스쳐 생성

public:

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() { return mSrvHeapBegin; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandle() { return mUavHeapBegin; }

	ScratchImage& GetOriginalImage() { return mOriginalImage; }
	ComPtr<ID3D12Resource> GetTex2D() { return mImage; }

public:
	float GetWidth() { return static_cast<float>(mDescription.Width); }
	float GetHeight() { return static_cast<float>(mDescription.Height); }

	// HeightMap에서 특정 좌표의 높이 값을 가져오는 함수
	float GetHeightValue(float u, float v) const;
	float GetHeightValuePixel(uint32 x, uint32 y) const;

	void SetRtvHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mRtvHeapBegin = handle; }
	void SetDsvHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mDsvHeapBegin = handle; }
	void SetSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mSrvHeapBegin = handle; }
	void SetUavHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mUavHeapBegin = handle; }

	void SetSrvIndex(uint32 index) { mSrvIndex = index; }
	uint32 GetSrvIndex() { return mSrvIndex; }

	void SetUavIndex(uint32 index) { mUavIndex = index; }
	uint32 GetUavIndex() { return mUavIndex; }

	void SetImageIndex(uint32 index) { mImageMapIndex = index; }
	uint32 GetImageIndex() { return mImageMapIndex; }

	void SetCubeMapIndex(uint32 index) { mCubeMapIndex = index; }
	uint32 GetCubeMapIndex() const { return mCubeMapIndex; }
	bool IsCubeMap() const { return mIsCubeMap; }
private:
	ScratchImage			 		mOriginalImage;
	D3D12_RESOURCE_DESC				mDescription{};

	ComPtr<ID3D12Resource>			mImage;
	uint32							mMipLevels;
private:
	D3D12_CPU_DESCRIPTOR_HANDLE		mSrvHeapBegin{};
	D3D12_CPU_DESCRIPTOR_HANDLE		mUavHeapBegin{};
	D3D12_CPU_DESCRIPTOR_HANDLE		mRtvHeapBegin{};
	D3D12_CPU_DESCRIPTOR_HANDLE		mDsvHeapBegin{};
	
	bool	mIsMSAA{};
	int 	mMSAACount{};
	TextureType mUse{ TextureType::TEXTURE_2D };

	uint32 mCubeMapIndex{};
	bool mIsCubeMap{};
private:
	uint32 mSrvIndex{};
	uint32 mUavIndex{};

	uint32 mImageMapIndex{};						// Root Signautre 내에 몇번째 인덱스인지
	static 	uint32			mTextureCount;			// Root Signautre 내에 textureMap 개수
	static uint32			mCubeMapCount;
};

