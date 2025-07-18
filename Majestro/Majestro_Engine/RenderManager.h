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
	void Render();
	void EndRender();

	void ResizeWindow(int32 width, int32 height);

	void CreateRenderTargetGroups();

	const WindowInfo& GetWindow() { return mWindow; }
public:

	SceneManager& GetSceneManager() { return *mSceneManager; }

	ID3D12DescriptorHeap* GetLegacyGraphicsDescriptorHeap() { return mGraphicsDescHeap->GetDescriptorHeap().Get(); }

public:
	shared_ptr<Device>					GetDevice()				{ return mDevice; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	//shared_ptr< ComputeCommandQueue>	GetComputeCmdQueue()	{ return mComputeCmdQueue; }

	shared_ptr<SwapChain>				GetSwapChain()			{ return mSwapChain; }
	
	shared_ptr<GraphicsDescriptorHeap> GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }
	//shared_ptr< ComputeDescriptorHeap>	GetComputeDescHeap()	{ return mComputeDescHeap; }


	shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_BUFFER_TYPE type) { return mConstantBuffer[static_cast<uint8>(type)]; }
	shared_ptr<RenderTarget> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return mRenderTargetGroups[static_cast<uint8>(type)]; }

private:

	unique_ptr<SceneManager> mSceneManager;

private:
	shared_ptr<Device>					mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>	mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	
	shared_ptr<SwapChain>				mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>	mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();
	vector<shared_ptr<ConstantBuffer>>		mConstantBuffer;

	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();

	array<shared_ptr<RenderTarget>, RENDER_TARGET_GROUP_COUNT> mRenderTargetGroups;
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

