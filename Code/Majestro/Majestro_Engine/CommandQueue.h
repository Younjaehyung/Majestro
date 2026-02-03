#pragma once

#include "SwapChain.h"
class Device;

class GraphicsCommandQueue
{

public:

	void Initialize (ComPtr<ID3D12Device> device, shared_ptr<SwapChain> swapChain);
	void WaitForGpuComplete();	//fence 기다림 용도 함수
	void FlushResourceCommandQueue();	//리소스 로딩용 CMD실행 함수



	void RenderBegin(uint32 frameIndex);
	void RenderEnd();

	void SignalFrame(uint32 frameIndex, uint32 backBufferIndex);
	void WaitForFrame(uint32 frameIndex);
	void WaitForBackBuffer(uint32 backBufferIndex);
	void WaitForFence(ID3D12Fence* fence, uint64 fenceValue);

	void CreateCommandQueue();


	D3D12_RESOURCE_BARRIER&			  GetBarrier() { return mBarrier; }
	ComPtr<ID3D12CommandQueue>		  GetCommandQueue() { return mCommandQueue; }
	ComPtr<ID3D12GraphicsCommandList> GetGraphicsCmdList() { return mCommandList; }
	ComPtr<ID3D12GraphicsCommandList> GetResourceCmdList() { return	mResourceCommandList; }

private:

	ComPtr<ID3D12CommandQueue>			mCommandQueue;
	array<ComPtr<ID3D12CommandAllocator>, FRAMEGROUP_COUNT> mCommandAllocators;
	ComPtr<ID3D12GraphicsCommandList>	mCommandList;
	
	//리소스 로딩용 CMD
	//기본 CMD는 RENDER할 때 begin과 end을 통해 사용되므로
	//그 이전에 데이터를 로딩할 때 사용하기 위한 추가 CMD리스트.
	ComPtr<ID3D12CommandAllocator>		mResourceCommandAlloc;
	ComPtr<ID3D12GraphicsCommandList>	mResourceCommandList;

	// Fence : 울타리(?)
	// 큐의 순서부여 후 어디까지 할지 정함
	// CPU / GPU 동기화를 위한 간단한 도구
	ComPtr<ID3D12Fence>					mFence;
	uint64								mFenceValue = 0;
	HANDLE								mFenceEvent = INVALID_HANDLE_VALUE;

	array<uint64, FRAMEGROUP_COUNT>		mFrameFenceValues{};
	array<uint64, SWAP_CHAIN_BUFFER_COUNT> mBackBufferFenceValues{};
	array<D3D12_RESOURCE_STATES, SWAP_CHAIN_BUFFER_COUNT> mBackBufferStates{};

	D3D12_RESOURCE_BARRIER		mBarrier;
	shared_ptr<SwapChain>		mSwapChain;
	ComPtr<ID3D12Device>		mDevice;
};


class ComputeCommandQueue
{
public:
	~ComputeCommandQueue();

	void Initialize(ComPtr<ID3D12Device> device);
	void WaitForGpuComplete();
	void FlushComputeCommandQueue();
	uint64 ExecuteCommandList(uint32 frameIndex);
	void ResetCommandList(uint32 frameIndex);

	ComPtr<ID3D12CommandQueue> GetCmdQueue() { return mCommandQueue; }
	ComPtr<ID3D12GraphicsCommandList> GetComputeCmdList() { return mCommandList; }
	ComPtr<ID3D12Fence> GetFence() { return mFence; }
private:
	ComPtr<ID3D12CommandQueue>			mCommandQueue;
	array<ComPtr<ID3D12CommandAllocator>, FRAMEGROUP_COUNT> mCommandAllocators;
	ComPtr<ID3D12GraphicsCommandList>	mCommandList;
	array<uint64, FRAMEGROUP_COUNT>		mFrameFenceValues{};

	ComPtr<ID3D12Fence>					mFence;
	uint32								mFenceValue = 0;
	HANDLE								mFenceEvent = INVALID_HANDLE_VALUE;
};