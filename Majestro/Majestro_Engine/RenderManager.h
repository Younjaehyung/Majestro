#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Buffer.h"
#include "SwapChain.h"
#include "RenderTarget.h"

class SceneManager;


struct GroupBuffer {
	shared_ptr<ConstantBuffer> PassInfo;
	shared_ptr<StructuredBuffer> LightInfo;
	shared_ptr<StructuredBuffer> ObjectInfo;
	shared_ptr<StructuredBuffer> MaterialInfo;

};

struct ParticleBuffer {
	
	shared_ptr<StructuredBuffer> Particle;
	shared_ptr<StructuredBuffer> RWParticleShared;//공유 전역변수
};


class RenderManager
{
public:
	void Initialize(const WindowInfo& info);


	void Update();

	void StartRender();
	void Render() {};
	void EndRender();

	void ResizeWindow(int32 width, int32 height);

	

	const WindowInfo& GetWindow() { return mWindow; }
public:


	ID3D12DescriptorHeap* GetLegacyGraphicsDescriptorHeap() { return mGraphicsDescHeap->GetDescriptorHeap().Get(); }

public:
	shared_ptr<Device>					GetDevice()				{ return mDevice; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	//shared_ptr< ComputeCommandQueue>	GetComputeCmdQueue()	{ return mComputeCmdQueue; }

	shared_ptr<SwapChain>				GetSwapChain()			{ return mSwapChain; }
	
	shared_ptr<GraphicsDescriptorHeap>	GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }

	//shared_ptr< ComputeDescriptorHeap>	GetComputeDescHeap()	{ return mComputeDescHeap; }




	//shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_INDEX type, uint8 count ) { return mConstantBuffer[count][static_cast<uint8>(type)- static_cast<uint8>(RCONSTANT_INDEX::RCONSTANT_INDEX_END)]; }
	//array < vector<shared_ptr<ConstantBuffer>>,GROUP_COUNT>& GetConstantBuffers() { return mConstantBuffer; }
	//
	//shared_ptr<StructuredBuffer> GetStructuredBuffer(STRUCTURED_INDEX type, uint8 count) { return mDynamicStructuredBuffer[count][static_cast<uint8>(type)]; }
	//array < vector<shared_ptr<StructuredBuffer>>, GROUP_COUNT>& GetStructuredBuffer() { return mDynamicStructuredBuffer; }


	uint8 GetFrameResourceIndex() {return mFrameResourceIndex;}
	uint8 GetFrameCurrIndex() {return mFrameCurrIndex;}

	shared_ptr<RenderTarget> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return mRenderTargetGroups[static_cast<uint8>(type)]; }

private:
	void CreateRenderTargetGroups();

	void CreateGlobal();
	void CreateGroup();
	void CreateParticle();
private:
	uint8			mFrameResourceIndex;	// 프레임리소스 그룹 인덱스 (현재 CPU에서 처리중인 Index)
	uint8			mFrameCurrIndex;		// 현재 GPU로 보낸 Index

private:
	// DX12
	shared_ptr<Device>							mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>			mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	
	shared_ptr<SwapChain>						mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>			mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();



	// ResourceBuffer


	array< GroupBuffer, FRAMEGROUP_COUNT>							mGroupBuffer;

	array< shared_ptr<StructuredBuffer>, FRAMEGROUP_COUNT>			mParticleShared; // 속성값(SRV)								
	array< ParticleBuffer, PARTICLE_GROUP_COUNT>				mParticleBuffer;


	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();

	// RenderTarget
	array<shared_ptr<RenderTarget>, RENDER_TARGET_GROUP_COUNT> mRenderTargetGroups;
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

