#include "pch.h"
#include "CommandQueue.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Buffer.h"
#include "SwapChain.h"

void GraphicsCommandQueue::Initialize(ComPtr<ID3D12Device> device, shared_ptr<SwapChain> swapChain)
{
	mSwapChain = swapChain;
	mDevice = device;
	CreateCommandQueue();

	mBackBufferStates.fill(D3D12_RESOURCE_STATE_PRESENT);
}

void GraphicsCommandQueue::WaitForGpuComplete()
{
	// cpu가 gpu의 현재 실행 상태에 따라 대기되는 상황이 발생됨.
	// 일단은 간단하게 이렇게 구성함

	// Advance the fence value to mark commands up to this fence point.
	mFenceValue++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mFenceValue);

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mFenceValue)
	{
		// Fire event when GPU hits current fence.  
		mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);

		// Wait until the GPU hits current fence event is fired.
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void GraphicsCommandQueue::RenderBegin(uint32 frameIndex)
{
	mCommandAllocators[frameIndex]->Reset();
	mCommandList->Reset(mCommandAllocators[frameIndex].Get(), nullptr);
	// 기존 정보들 클리어

	uint32 backIndex = mSwapChain->GetBackBufferIndex();



	D3D12_RESOURCE_STATES& currentState = mBackBufferStates[backIndex];
	if (currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
			currentState,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		//더블버퍼링을 위해 기존의 출력되던 버퍼를 후방버퍼로 바꾸겠다


		//SetGraphicsRootDescriptorTable와 거의 세트임. (1.선택후 2.명령어로 보내기.)
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &mBarrier);
		currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}


}

void GraphicsCommandQueue::RenderEnd()
{
	
		uint32 backIndex = mSwapChain->GetBackBufferIndex();

		if (RENDERMANAGER.IsMsaaEnabled()) // msaa
		{
			

			auto& msaaGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::FINAL));
			auto& swapChainGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN));

			ID3D12Resource* msaaResource = msaaGroup.GetRTTexture(backIndex)->GetTex2D().Get();
			ID3D12Resource* backBuffer = swapChainGroup.GetRTTexture(backIndex)->GetTex2D().Get();

			D3D12_RESOURCE_BARRIER preResolveBarriers[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(
					msaaResource,
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(
					backBuffer,
					mBackBufferStates[backIndex],
					D3D12_RESOURCE_STATE_RESOLVE_DEST)
			};

			mCommandList->ResourceBarrier(_countof(preResolveBarriers), preResolveBarriers);
			mBackBufferStates[backIndex] = D3D12_RESOURCE_STATE_RESOLVE_DEST;

			mCommandList->ResolveSubresource(backBuffer, 0, msaaResource, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

			D3D12_RESOURCE_BARRIER postResolveBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				msaaResource,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				D3D12_RESOURCE_STATE_COMMON);
			mCommandList->ResourceBarrier(1, &postResolveBarrier);
		}

		//MSAA가 아닐경우 그냥 넘어감

		D3D12_RESOURCE_STATES& currentState = mBackBufferStates[backIndex];
		if (currentState != D3D12_RESOURCE_STATE_PRESENT)
		{
			mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
				currentState,
				D3D12_RESOURCE_STATE_PRESENT);

			mCommandList->ResourceBarrier(1, &mBarrier);
			currentState = D3D12_RESOURCE_STATE_PRESENT;
		}
		mCommandList->Close();
		// 명령어들을 큐에 넣고 close을 한 다음에 실행하게 되는데 Reset하면 알아서 Open이 됨

		// 커맨드 리스트 수행
		ID3D12CommandList* cmdListArr[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);
		//바로 실행되지는 않는다. GPU에서 때가 되면 명령들을 실행하게 된다.
		//GPU 큐에 명령들을 집어넣는 것.

		mSwapChain->Present();// 백 버퍼(B)와 프론트 버퍼(A) 교체



}

void GraphicsCommandQueue::SignalFrame(uint32 frameIndex, uint32 backBufferIndex)
{
	mFenceValue++;
	mFrameFenceValues[frameIndex] = mFenceValue;
	mBackBufferFenceValues[backBufferIndex] = mFenceValue;
	mCommandQueue->Signal(mFence.Get(), mFenceValue);
}

void GraphicsCommandQueue::WaitForFrame(uint32 frameIndex)
{
	const uint64 fenceValue = mFrameFenceValues[frameIndex];
	if (fenceValue == 0)
		return;

	if (mFence->GetCompletedValue() < fenceValue)
	{
		mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void GraphicsCommandQueue::WaitForFence(ID3D12Fence* fence, uint64 fenceValue)
{
	if (fence == nullptr || fenceValue == 0)
		return;

	mCommandQueue->Wait(fence, fenceValue);
}

void GraphicsCommandQueue::WaitForBackBuffer(uint32 backBufferIndex)
{
	const uint64 fenceValue = mBackBufferFenceValues[backBufferIndex];
	if (fenceValue == 0)
		return;

	if (mFence->GetCompletedValue() < fenceValue)
	{
		mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void GraphicsCommandQueue::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// CmQ의 타입을 설정
	// D3D12_COMMAND_LIST_TYPE_DIRECT : GPU가 직접 실행할 수 있는 명령 버퍼(리스트)
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// CmQ의 플래그 설정
	// D3D12_COMMAND_QUEUE_FLAG_NONE : 기본 명령 큐

	mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
	//cmdQueue 생성

	// - D3D12_COMMAND_LIST_TYPE_DIRECT : GPU가 직접 실행하는 명령 목록
	for (auto& commandAllocator : mCommandAllocators)
	{
		mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	}


	// GPU가 하나인 시스템에서는 0으로
	// DIRECT or BUNDLE
	// Allocator
	// 초기 상태 (그리기 명령은 nullptr 지정)
	// cmdList 생성
	mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));
	// CommandList는 Close / Open 상태가 있는데
	// Open 상태에서 Command를 넣다가 Close한 다음 제출하는 개념
	mCommandList->Close();

	//리소스CMD 용////
	mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mResourceCommandAlloc));
	mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mResourceCommandAlloc.Get(), nullptr, IID_PPV_ARGS(&mResourceCommandList));
	//////////////////

	// CreateFence
	// - CPU와 GPU의 동기화 수단으로 쓰인다
	mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GraphicsCommandQueue::FlushResourceCommandQueue()
{
	mResourceCommandList->Close();

	ID3D12CommandList* cmdListArr[] = { mResourceCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	WaitForGpuComplete();

	mResourceCommandAlloc->Reset();
	mResourceCommandList->Reset(mResourceCommandAlloc.Get(), nullptr);
}



// ************************
// ComputeCommandQueue
// ************************

ComputeCommandQueue::~ComputeCommandQueue()
{
	::CloseHandle(mFenceEvent);
}

void ComputeCommandQueue::Initialize(ComPtr<ID3D12Device> device)
{
	D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
	computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&mCommandQueue));

	for (auto& commandAllocator : mCommandAllocators)
	{
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocator));
	}
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));

	// CreateFence
	// - CPU와 GPU의 동기화 수단으로 쓰인다
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void ComputeCommandQueue::WaitForGpuComplete()
{
	const uint64 fenceValue = mFenceValue;
	if (fenceValue == 0)
		return;

	//if (mFence->GetCompletedValue() < mFenceValue)
	//{
	//	mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	//	::WaitForSingleObject(mFenceEvent, INFINITE);
	//}

	if (mFence->GetCompletedValue() < fenceValue)
	{
		mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}



void ComputeCommandQueue::FlushComputeCommandQueue()
{
	//mCommandList->Close();

	//ID3D12CommandList* cmdListArr[] = { mCommandList.Get() };
	//auto t = _countof(cmdListArr);
	//mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	//WaitForGpuComplete();

	//mCommandAllocator->Reset();
	//mCommandList->Reset(mCommandAllocator.Get(), nullptr);
	const uint32 frameIndex = gEngine->GetRenderManager().GetFrameResourceIndex();
	ExecuteCommandList(frameIndex);
	WaitForGpuComplete();

}

uint64 ComputeCommandQueue::ExecuteCommandList(uint32 frameIndex)
{
	const uint64 fenceValue = mFrameFenceValues[frameIndex];
	if (fenceValue != 0 && mFence->GetCompletedValue() < fenceValue)
	{
		mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
	mCommandList->Close();

	ID3D12CommandList* cmdListArr[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	mFenceValue++;
	mCommandQueue->Signal(mFence.Get(), mFenceValue);
	mFrameFenceValues[frameIndex] = mFenceValue;
	ResetCommandList(frameIndex);

	return mFenceValue;
}


void ComputeCommandQueue::ResetCommandList(uint32 frameIndex)
{
	mCommandAllocators[frameIndex]->Reset();
	mCommandList->Reset(mCommandAllocators[frameIndex].Get(), nullptr);
}