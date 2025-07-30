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
	
	shared_ptr<GraphicsDescriptorHeap> GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }
	//shared_ptr< ComputeDescriptorHeap>	GetComputeDescHeap()	{ return mComputeDescHeap; }


	shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_BUFFER_TYPE type) { return mConstantBuffer[static_cast<uint8>(type)]; }
	vector<shared_ptr<ConstantBuffer>>& GetConstantBuffers() { return mConstantBuffer; }	
	
	shared_ptr<StructuredBuffer> GetStructuredBuffer(CONSTANT_BUFFER_TYPE type) { return mConstantBuffer[static_cast<uint8>(type)]; }
	vector<shared_ptr<StructuredBuffer>>& GetStructuredBuffer() { return mConstantBuffer; }



	shared_ptr<RenderTarget> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return mRenderTargetGroups[static_cast<uint8>(type)]; }

private:
	void CreateRenderTargetGroups();
	void CreateConstantBuffer(CBV_REGISTER reg, uint32 bufferSize, uint32 count);	//Constant버퍼 생성 (
	void CreateStructuredBuffer(uint32 elementSize, uint32 elementCount);	//Structure버퍼 생성

private:
	// buffer
	ComPtr<ID3D12Resource>	LIghtBuffer;
	ComPtr<ID3D12Resource>	ObjectBuffer;
	ComPtr<ID3D12Resource>	MaterialBuffer;

private:
	// DX12
	shared_ptr<Device>							mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>			mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	
	shared_ptr<SwapChain>						mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>			mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();



	// ResourceBuffer
	vector<shared_ptr<ConstantBuffer>>							mConstantBuffer;	// 0: Camera 나머지 : 추후 추가될 constantBuffer용

	vector< shared_ptr<ShaderResourceBuffer>>					mTextureBuffer;	// 머티리얼을 위한 텍스쳐 버퍼

	array< vector<shared_ptr<StructuredBuffer>>, GROUP_COUNT>	mDynamicStructuredBuffer;	// 프레임 리소스를 위한 GROUP_COUNT만큼의 동적 버퍼
	vector<shared_ptr<StructuredBuffer>>						mStaticStructuredBuffer;	// 머티리얼, 본 계층을 위한 정적 버퍼

	

	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();

	// RenderTarget
	array<shared_ptr<RenderTarget>, RENDER_TARGET_GROUP_COUNT> mRenderTargetGroups;
private:
	uint8			mGroupIndex;	// 프레임리소스 그룹 인덱스

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

