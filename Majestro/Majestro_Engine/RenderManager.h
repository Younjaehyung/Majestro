#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Buffer.h"
#include "SwapChain.h"
#include "RenderTarget.h"

class SceneManager;


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




	shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_INDEX type, uint8 count ) { return mConstantBuffer[count][static_cast<uint8>(type)- static_cast<uint8>(RCONSTANT_INDEX::RCONSTANT_INDEX_END)]; }
	array < vector<shared_ptr<ConstantBuffer>>,GROUP_COUNT>& GetConstantBuffers() { return mConstantBuffer; }
	
	shared_ptr<StructuredBuffer> GetStructuredBuffer(STRUCTURED_INDEX type, uint8 count) { return mDynamicStructuredBuffer[count][static_cast<uint8>(type)]; }
	array < vector<shared_ptr<StructuredBuffer>>, GROUP_COUNT>& GetStructuredBuffer() { return mDynamicStructuredBuffer; }


	uint8 GetFrameResourceIndex() {return mFrameResourceIndex;}

	shared_ptr<RenderTarget> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return mRenderTargetGroups[static_cast<uint8>(type)]; }

private:
	void CreateRenderTargetGroups();
	void CreateConstantBuffer(CONSTANT_INDEX type, uint32 bufferSize);	//Constant버퍼 생성 (
	void CreateStructuredBuffer(STRUCTURED_INDEX type, uint32 elementSize, uint32 elementCount);	//Structure버퍼 생성

private:
	uint8			mFrameResourceIndex;	// 프레임리소스 그룹 인덱스
	

private:
	// DX12
	shared_ptr<Device>							mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>			mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	
	shared_ptr<SwapChain>						mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>			mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();



	// ResourceBuffer
	array< vector<shared_ptr<ConstantBuffer>>, GROUP_COUNT>		mConstantBuffer;	// 0: Camera 나머지 : 추후 추가될 constantBuffer용

	array< vector<shared_ptr<StructuredBuffer>>, GROUP_COUNT>	mDynamicStructuredBuffer;	// 프레임 리소스를 위한 GROUP_COUNT만큼의 동적 버퍼
	vector<shared_ptr<StructuredBuffer>>						mStaticStructuredBuffer;	// 머티리얼, 본 계층을 위한 정적 버퍼

	

	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();

	// RenderTarget
	array<shared_ptr<RenderTarget>, RENDER_TARGET_GROUP_COUNT> mRenderTargetGroups;
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

