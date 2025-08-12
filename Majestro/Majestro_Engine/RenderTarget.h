#pragma once
#include "Texture.h"

enum class RENDER_TARGET_GROUP_TYPE : uint8
{
	SWAP_CHAIN, // BACK_BUFFER, FRONT_BUFFER
	SHADOW, // SHADOW
	G_BUFFER, // POSITION, NORMAL, COLOR 
	LIGHTING, // DIFFUSE LIGHT, SPECULAR LIGHT
	END,
};

enum
{
	RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT = 1,
	RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT = 3,
	RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT = 2,
	RENDER_TARGET_GROUP_COUNT = static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::END)
};

struct RenderTargetStruct
{
	shared_ptr<Texture> Target;
	float ClearColor[4] = {0.0f,0.0f ,0.0f ,0.0f };
};

class RenderTarget
{
public:
	void Initialize(shared_ptr<Texture> dsTexture);
	void Create(RENDER_TARGET_GROUP_TYPE groupType, RenderTargetStruct& rtVec, uint8 type);
	

	void OMSetRenderTargets(uint8 type,uint32 count, uint32 offset);
	void OMSetRenderTargets(uint8 type);

	void ClearRenderTargetView(uint32 index);
	void ClearRenderTargetView();

	shared_ptr<Texture> GetRTTexture(uint32 index) { return mRenderTargetVec[index].Target; }
	shared_ptr<Texture> GetDSTexture() { return mDepthStencilTexture; }

	void WaitTargetToResource();
	void WaitResourceToTarget();
public:
	

private:
	vector<RenderTargetStruct>		mRenderTargetVec;
	RENDER_TARGET_GROUP_TYPE		mRenderTargetType;
	uint32							mRenderTargetCount;
		//G_BUFFER 텍스쳐

	ComPtr<ID3D12DescriptorHeap>	mRenderTargetHeap;
	ComPtr<ID3D12DescriptorHeap>	mDepthStencilHeap;

	shared_ptr<Texture>				mDepthStencilTexture;
private:
	uint32							mRtvHeapSize;
	D3D12_CPU_DESCRIPTOR_HANDLE		mRtvHeapBegin;
	D3D12_CPU_DESCRIPTOR_HANDLE		mDsvHeapBegin;

private:
	int i = 0;

	D3D12_RESOURCE_BARRIER			mTargetToResource[8];	//타켓에서 리소스로 넘어갈때의 베리어
	D3D12_RESOURCE_BARRIER			mResourceToTarget[8];	//리소스에서 랜더타켓으로 변환시 넘어가는 베리어
};
