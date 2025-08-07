#pragma once
#include "Object.h"

class Texture : public Object
{
public:
	Texture();
	virtual ~Texture();

	virtual void Load(const wstring& path);	//리소스 이미지 로딩


	void Create(DXGI_FORMAT format, uint32 width, uint32 height,
		const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
		D3D12_RESOURCE_FLAGS resFlags, Vec4 clearColor = Vec4());
	//새로운 사용자 텍스쳐 생성

	void CreateFromResource(ComPtr<ID3D12Resource> tex2D);
	//버퍼를 이용해서 텍스쳐 생성

public:

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() { return mSrvHeapBegin; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandle() { return mUavHeapBegin; }

	ScratchImage& GetOriginalImage() { return mOriginalImage; }
	ComPtr<ID3D12Resource> GetTex2D() { return mImage; }

public:
	float GetWidth() { return static_cast<float>(mDescription.Width); }
	float GetHeight() { return static_cast<float>(mDescription.Height); }


	void SetRtvHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mRtvHeapBegin = handle; }
	void SetDsvHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mDsvHeapBegin = handle; }

	uint32 SetSrvIndex(uint32 index) { mSrvIndex = index; }
	uint32 GetSrvIndex() { return mSrvIndex; }

	uint32 SetUavIndex(uint32 index) { mUavIndex = index; }
	uint32 GetUavIndex() { return mUavIndex; }

	uint32 SetImageIndex(uint32 index) { mImageMapIndex = index; }
	uint32 GetImageIndex() { return mImageMapIndex; }
private:
	ScratchImage			 		mOriginalImage;
	D3D12_RESOURCE_DESC				mDescription;

	ComPtr<ID3D12Resource>			mImage;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE		mSrvHeapBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE		mUavHeapBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE		mRtvHeapBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE		mDsvHeapBegin = {};

private:
	uint32 mSrvIndex{};
	uint32 mUavIndex{};

	uint32 mImageMapIndex{};
};

