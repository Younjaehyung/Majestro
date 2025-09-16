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
	shared_ptr<StructuredBuffer> InstanceInfo;
	shared_ptr<StructuredBuffer> LightInfo;
	shared_ptr<StructuredBuffer> ObjectInfo;
	shared_ptr<StructuredBuffer> ParticleInfo;
	shared_ptr<StructuredBuffer> AnimationInfo;

};

struct ParticleBuffer {
	
	shared_ptr<StructuredBuffer> Particle;
	shared_ptr<StructuredBuffer> RWParticleShared;//공유 전역변수
};

struct AnimationBuffer {

	shared_ptr<StructuredBuffer> SkeletonBone;
	shared_ptr<StructuredBuffer> AnimationClip;
	shared_ptr<StructuredBuffer> AnimationMeta;
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

	void SetTable();

	const WindowInfo& GetWindow() { return mWindow; }
	ID3D12DescriptorHeap*				GetLegacyGraphicsDescriptorHeap() { return mGraphicsDescHeap->GetDescriptorHeap().Get(); }

public:
	shared_ptr<Device>					GetDevice()				{ return mDevice; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	shared_ptr<ComputeCommandQueue>		GetComputeCmdQueue()	{ return mComputeCommandQueue; }

	shared_ptr<SwapChain>				GetSwapChain()			{ return mSwapChain; }
	
	shared_ptr<GraphicsDescriptorHeap>	GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }
	shared_ptr<RenderTargetHeap>		GetRenderTargetHeap()	{ return mRenderTargetHeap; }

public:

	shared_ptr<GroupBuffer>	&			GetGroupBuffer(uint32 frame)		{ return mGroupBuffer[frame]; }
	shared_ptr<ParticleBuffer>&			GetParticleBuffers(uint32 group)	{ return mParticleBuffer[group]; }
	shared_ptr<AnimationBuffer>&		GetAnimationBuffers()	{ return mAnimationBuffer; }
	shared_ptr<StructuredBuffer>&		GetMaterialBuffers()				{ return mMaterialBuffer; }

	RenderTargetGroup&					GetRenderTargetGroup(uint8 type)	{ return mRenderTargetGroup[type]; }

public:
	uint32 GetFrameResourceIndex() {return mFrameResourceIndex;}
	uint32 GetFrameCurrIndex() {return mFrameCurrIndex;}

	
private:
	void CreateRenderTargetGroups();

	void CreateGlobal();
	void CreateGroup();
	void CreateMaterial();
	void CreateParticle();
	void CreateAnimation();
private:
	uint32			mFrameResourceIndex{};	// 프레임리소스 그룹 인덱스 (현재 CPU에서 처리중인 Index)
	uint32			mFrameCurrIndex{};		// 현재 GPU로 보낸 Index

private:
	// DX12
	shared_ptr<Device>							mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>			mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	shared_ptr<ComputeCommandQueue>				mComputeCommandQueue		= make_shared<ComputeCommandQueue>();
	
	
	shared_ptr<SwapChain>						mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>			mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();

	shared_ptr<RenderTargetHeap>				mRenderTargetHeap			= make_shared<RenderTargetHeap>();

	shared_ptr<RootSignature>					mRootSignature;
private:


	array <shared_ptr<GroupBuffer>, FRAMEGROUP_COUNT>				mGroupBuffer;
	array <shared_ptr<ParticleBuffer>, PARTICLE_GROUP_COUNT>		mParticleBuffer;
	shared_ptr<AnimationBuffer>										mAnimationBuffer;
	shared_ptr<StructuredBuffer>									mMaterialBuffer;

	array< RenderTargetGroup, RENDER_TARGET_GROUP_COUNT>			mRenderTargetGroup;
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

