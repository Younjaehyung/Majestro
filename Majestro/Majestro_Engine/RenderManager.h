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
	shared_ptr<StructuredBuffer> ParticleInfo;

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


	array <GroupBuffer, FRAMEGROUP_COUNT>& GetGroupBuffer() { return mGroupBuffer; }
	array <  ParticleBuffer, PARTICLE_GROUP_COUNT>& GetConstantBuffers() { return mParticleBuffer; }
	


	uint8 GetFrameResourceIndex() {return mFrameResourceIndex;}
	uint8 GetFrameCurrIndex() {return mFrameCurrIndex;}

	shared_ptr<RenderTarget> GetRTGroup() { return mGBufferTarget; }

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


private:
	// ResourceBuffer
	array< GroupBuffer, FRAMEGROUP_COUNT>							mGroupBuffer;						
	array< ParticleBuffer, PARTICLE_GROUP_COUNT>					mParticleBuffer;

	shared_ptr<RenderTarget> mGBufferTarget = make_shared<RenderTarget>();
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

