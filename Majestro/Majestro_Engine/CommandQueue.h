#pragma once

class SwapChain;
class Device;

class GraphicsCommandQueue
{

public:

	void Initialize (ComPtr<ID3D12Device> device, shared_ptr<SwapChain> swapChain);
	void WaitForGpuComplete();	//fence 기다림 용도 함수
	void FlushResourceCommandQueue();	//리소스 로딩용 CMD실행 함수

	void RenderBegin();
	void RenderEnd();

	void CreateCommandQueue();

	ComPtr<ID3D12CommandQueue>		  GetCommandQueue() { return mCommandQueue; }
	ComPtr<ID3D12GraphicsCommandList> GetGraphicsCmdList() { return mCommandList; }
	ComPtr<ID3D12GraphicsCommandList> GetResourceCmdList() { return	mResourceCommandList; }

private:

	ComPtr<ID3D12CommandQueue>			mCommandQueue;
	ComPtr<ID3D12CommandAllocator>		mCommandAllocator;
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
	uint32								mFenceValue = 0;
	HANDLE								mFenceEvent = INVALID_HANDLE_VALUE;

	shared_ptr<SwapChain>		mSwapChain;
	ComPtr<ID3D12Device>		mDevice;
};

