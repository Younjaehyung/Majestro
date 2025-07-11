#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "RootSignatureParams.h"
#include "SwapChain.h"

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

	const WindowInfo& GetWindow() { return mWindow; }
public:

	SceneManager& GetSceneManager() { return *mSceneManager; }


public:
	shared_ptr< Device>					GetDevice()				{ return mDevice; }
	shared_ptr< GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	//shared_ptr< ComputeCommandQueue>	GetComputeCmdQueue()	{ return mComputeCmdQueue; }

	shared_ptr< SwapChain>				GetSwapChain()			{ return mSwapChain; }
	//shared_ptr< RootSignature>			GetRootSignature()		{ return mRootSignature; }
	//shared_ptr< GraphicsDescriptorHeap> GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }
	//shared_ptr< ComputeDescriptorHeap>	GetComputeDescHeap()	{ return mComputeDescHeap; }


	//shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_BUFFER_TYPE type) { return _constantBuffers[static_cast<uint8>(type)]; }
	//shared_ptr<RenderTargetGroup> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return _rtGroups[static_cast<uint8>(type)]; }


private:

	unique_ptr<SceneManager> mSceneManager;

private:
	shared_ptr<Device>					mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>	mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();
	shared_ptr<SwapChain>				mSwapChain					= make_shared<SwapChain>();
	//shared_ptr<RootSignature>			mRootSignature		= make_shared<RootSignature>();
	//shared_ptr<GraphicsDescriptorHeap>	mGraphicsDescHeap	= make_shared<GraphicsDescriptorHeap>();
	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();



private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

