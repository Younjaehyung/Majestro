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
	ComPtr<ID3D12DescriptorHeap> GetSRV() { return mSrvHeap; }
	ComPtr<ID3D12DescriptorHeap> GetRTV() { return mRtvHeap; }
	ComPtr<ID3D12DescriptorHeap> GetDSV() { return mDsvHeap; }
	ComPtr<ID3D12DescriptorHeap> GetUAV() { return mUavHeap; }
public:
	float GetWidth() { return static_cast<float>(mDescription.Width); }
	float GetHeight() { return static_cast<float>(mDescription.Height); }

	

private:
	ScratchImage			 		mOriginalImage;
	D3D12_RESOURCE_DESC				mDescription;
	ComPtr<ID3D12Resource>			mImage;

	ComPtr<ID3D12DescriptorHeap>	mSrvHeap;	//리소스 이미지
	ComPtr<ID3D12DescriptorHeap>	mRtvHeap;
	ComPtr<ID3D12DescriptorHeap>	mDsvHeap;
	ComPtr<ID3D12DescriptorHeap>	mUavHeap;
private:
	D3D12_CPU_DESCRIPTOR_HANDLE		mSrvHeapBegin = {};
	D3D12_CPU_DESCRIPTOR_HANDLE		mUavHeapBegin = {};
};

