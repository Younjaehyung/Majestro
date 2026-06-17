#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "DescriptorBlockAllocator.h"
#include "Buffer.h"
#include "ParticleResourceAllocator.h"
#include "SwapChain.h"
#include "RenderTarget.h"

class SceneManager;


// 사용자 그래픽 설정
struct GraphicsSettings {
	bool bFog          = true;
	bool bGodRay       = true;
	bool bBloom        = true;
	bool bOutline      = true;
	bool bHBAO         = true;
	bool bFXAA         = true;
	bool bShadow       = true;
};


struct GroupBuffer {
	shared_ptr<ConstantBuffer> PassInfo;
	shared_ptr<StructuredBuffer> InstanceInfo;
	shared_ptr<StructuredBuffer> LightInfo;
	shared_ptr<StructuredBuffer> ObjectInfo;
	shared_ptr<StructuredBuffer> ParticleInfo;
	shared_ptr<StructuredBuffer> UIInfo;
	shared_ptr<StructuredBuffer> AnimInstanceInfo;
	shared_ptr<StructuredBuffer> AnimResultInfo;
	shared_ptr<StructuredBuffer> ForwardPlusTileMetaInfo;
	shared_ptr<StructuredBuffer> ForwardPlusLightIndexInfo;
	shared_ptr<StructuredBuffer> PassCustomTableInfo;  // pass별 커스텀 텍스처/파라미터 테이블


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
	void InitEffekseer(int32_t instanceMax = 8000, int32_t squareMaxCount = 2000);

	void Update();

	void StartRender();
	void Render() {};
	void EndRender();

	void CheckMsaaSupport(DXGI_FORMAT format, uint32 sampleCount);
	void ResizeWindow(int32 width, int32 height);

	void SetComputTable();
	void SetGraphicsTable();
	void SetTable();
	void SetAnimationComputeFenceValue(uint64 fenceValue) { mAnimationComputeFenceValue = fenceValue; }
	const WindowInfo& GetWindow() { return mWindow; }
	ID3D12DescriptorHeap*				GetLegacyGraphicsDescriptorHeap() { return mDescHeap->GetDescriptorHeap().Get(); }

	uint32 GetMsaaSampleCount() const { return mMsaaSampleCount; }
	uint32 GetMsaaQuality() const { return mMsaaQuality; }
	bool IsMsaaEnabled() const { return false /*mMsaaSampleCount > 1*/; }

	// 사용자 그래픽 설정
	GraphicsSettings&       GetGraphicsSettings()       { return mGraphicsSettings; }
	const GraphicsSettings& GetGraphicsSettings() const { return mGraphicsSettings; }

public:
	shared_ptr<Device>					GetDevice()				{ return mDevice; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	shared_ptr<ComputeCommandQueue>		GetComputeCmdQueue()	{ return mComputeCommandQueue; }

	shared_ptr<SwapChain>				GetSwapChain()			{ return mSwapChain; }
	
	shared_ptr<GraphicsDescriptorHeap>	GetGraphicsDescHeap()	{ return mDescHeap; }
	shared_ptr<RenderTargetHeap>		GetRenderTargetHeap()	{ return mRenderTargetHeap; }

	shared_ptr<DirectX::DX12::GraphicsMemory> GetGraphicsMemory() { return mGraphicsMemory; }

	EffekseerRenderer::RendererRef            GetEfkRendererHDR() { return mEfkRendererHDR; }
	EffekseerRenderer::RendererRef            GetEfkRendererUI()  { return mEfkRendererUI;  }
	Effekseer::Backend::GraphicsDeviceRef     GetEfkGraphicsDevice() { return mEfkGraphicsDevice; }

	D3D12_VIEWPORT&						GetViewPort() { return mViewport; }
	D3D12_RECT&							GetScissorRect() { return mScissorRect; }


public:

	shared_ptr<GroupBuffer>	&			GetGroupBuffer(uint32 frame)		{ return mGroupBuffer[frame]; }
	shared_ptr<AnimationBuffer>&		GetAnimationBuffers()				{ return mAnimationBuffer; }
	shared_ptr<StructuredBuffer>&		GetMaterialBuffers()				{ return mMaterialBuffer; }
	shared_ptr<StructuredBuffer>		GetParticleSpawnBuffer(uint32 frame) { return mParticleSpawnBuffer[frame]; }
	DescriptorBlockAllocator&			GetParticleDescriptorAllocator() { return mParticleDescriptorAllocator; }
	ParticleResourceAllocator&			GetParticleResourceAllocator() { return mParticleResourceAllocator; }

	RenderTargetGroup&					GetRenderTargetGroup(uint8 type)	{ return mRenderTargetGroup[type]; }

public:
	uint32 GetFrameResourceIndex() {return mFrameResourceIndex;}
	uint32 GetFrameCurrIndex() {return mFrameCurrIndex;}
	uint64 GetAnimationComputeFenceValue() const { return mAnimationComputeFenceValue; }
	uint64 GetCompletedGraphicsFenceValue() const { return mGraphicsCommandQueue->GetCompletedFenceValue(); }
	uint64 GetLastGraphicsFenceValue() const { return mGraphicsCommandQueue->GetLastSignaledFenceValue(); }
	
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
	uint64			mAnimationComputeFenceValue{};
private:
	// DX12
	shared_ptr<Device>							mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>			mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	shared_ptr<ComputeCommandQueue>				mComputeCommandQueue		= make_shared<ComputeCommandQueue>();
	
	
	shared_ptr<SwapChain>						mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>			mDescHeap					= make_shared<GraphicsDescriptorHeap>();

	shared_ptr<RenderTargetHeap>				mRenderTargetHeap			= make_shared<RenderTargetHeap>();
	shared_ptr<DirectX::DX12::GraphicsMemory>	mGraphicsMemory;
	shared_ptr<RootSignature>					mRootSignature;

	
private:
	array <shared_ptr<GroupBuffer>, FRAMEGROUP_COUNT>				mGroupBuffer;
	array <shared_ptr<StructuredBuffer>, FRAMEGROUP_COUNT>			mParticleSpawnBuffer;
	shared_ptr<AnimationBuffer>										mAnimationBuffer;
	shared_ptr<StructuredBuffer>									mMaterialBuffer;
	DescriptorBlockAllocator										mParticleDescriptorAllocator;	// 에미터용 디스크립터 슬롯 관리
	ParticleResourceAllocator										mParticleResourceAllocator;		// 에미터용 파티클 버퍼(GPU 메모리) 관리

	array< RenderTargetGroup, RENDER_TARGET_GROUP_COUNT>			mRenderTargetGroup;
private:

	uint32			mMsaaSampleCount{ 1 };
	uint32			mMsaaQuality{ 0 };

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
private:
	// Effekseer GPU 리소스
	Effekseer::Backend::GraphicsDeviceRef               mEfkGraphicsDevice;

	// 인게임 VFX
	EffekseerRenderer::RendererRef                      mEfkRendererHDR;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> mEfkMemoryPoolHDR;
	Effekseer::RefPtr<EffekseerRenderer::CommandList>   mEfkCmdListHDR;

	// UI VFX
	EffekseerRenderer::RendererRef                      mEfkRendererUI;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> mEfkMemoryPoolUI;
	Effekseer::RefPtr<EffekseerRenderer::CommandList>   mEfkCmdListUI;

private:
	GraphicsSettings mGraphicsSettings;   // 사용자 그래픽 설정

};

