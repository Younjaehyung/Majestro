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

void GraphicsCommandQueue::RenderBegin()
{
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), nullptr);
	// 기존 정보들 클리어

	uint32 backIndex = mSwapChain->GetBackBufferIndex();

	mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET); 
	//더블버퍼링을 위해 기존의 출력되던 버퍼를 후방버퍼로 바꾸겠다

		//SetGraphicsRootDescriptorTable와 거의 세트임. (1.선택후 2.명령어로 보내기.)
	D3D12_RESOURCE_BARRIER barrier = gEngine->GetRenderManager().GetGraphicsCmdQueue()->GetBarrier();
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
}

void GraphicsCommandQueue::RenderEnd()
{
	
		uint32 backIndex = mSwapChain->GetBackBufferIndex();

		mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			RENDERMANAGER.GetRenderTargetGroup(static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_PRESENT); 

		mCommandList->ResourceBarrier(1, &mBarrier);
		mCommandList->Close();
		// 명령어들을 큐에 넣고 close을 한 다음에 실행하게 되는데 Reset하면 알아서 Open이 됨

		// 커맨드 리스트 수행
		ID3D12CommandList* cmdListArr[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);
		//바로 실행되지는 않는다. GPU에서 때가 되면 명령들을 실행하게 된다.
		//GPU 큐에 명령들을 집어넣는 것.

		mSwapChain->Present();// 백 버퍼(B)와 프론트 버퍼(A) 교체

		// Wait until frame commands are complete.  This waiting is inefficient and is
		// done for simplicity.  Later we will show how to organize our rendering code
		// so we do not have to wait per frame.
		WaitForGpuComplete();

		mSwapChain->SwapIndex();
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
	mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
	//cmdAlloc 생성


	// GPU가 하나인 시스템에서는 0으로
	// DIRECT or BUNDLE
	// Allocator
	// 초기 상태 (그리기 명령은 nullptr 지정)
	// cmdList 생성
	mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList));

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
