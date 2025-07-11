#include "pch.h"
#include "CommandQueue.h"
#include "Engine.h"
#include "SwapChain.h"

void GraphicsCommandQueue::Initialize(ComPtr<ID3D12Device> device, shared_ptr<SwapChain> swapChain)
{
	mSwapChain = swapChain;
	mDevice = device;
	CreateCommandQueue();
}

void GraphicsCommandQueue::WaitForGpuComplete()
{
}

void GraphicsCommandQueue::RenderBegin()
{
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), nullptr);
	// 기존 정보들 클리어

	int8 backIndex = mSwapChain->GetBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET); 
	//더블버퍼링을 위해 기존의 출력되던 버퍼를 후방버퍼로 바꾸겠다

	mCommandList->SetGraphicsRootSignature(GRAPHICS_ROOT_SIGNATURE.Get());	//루트 시그니쳐 선언
	gEngine->GetConstantBuffer(CONSTANT_BUFFER_TYPE::TRANSFORM)->Clear();	//버퍼 클리어
	gEngine->GetConstantBuffer(CONSTANT_BUFFER_TYPE::MATERIAL)->Clear();

	gEngine->GetGraphicsDescHeap()->Clear();	//테이블힙 클리어



	ID3D12DescriptorHeap* descHeap = gEngine->GetGraphicsDescHeap()->GetDescriptorHeap().Get();
	mCommandList->SetDescriptorHeaps(1, &descHeap);	//몇번째 테이블힙을 사용할건지 선택함 (매우 무거움으로 프레임당 1번만 사용할것을 권장함)
	//SetGraphicsRootDescriptorTable와 거의 세트임. (1.선택후 2.명령어로 보내기.)

	mCommandList->ResourceBarrier(1, &barrier);
}

void GraphicsCommandQueue::RenderEnd()
{
	
		int8 backIndex = mSwapChain->GetBackBufferIndex();

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(backIndex)->GetTex2D().Get(),	//RenderTargetGroup에서 backbuffer을 가져옴
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_PRESENT); 

		mCommandList->ResourceBarrier(1, &barrier);
		mCommandList->Close();
		// 명령어들을 큐에 넣고 close을 한 다음에 실행하게 되는데 Reset하면 알아서 Open이 됨

		// 커맨드 리스트 수행
		ID3D12CommandList* cmdListArr[] = { _cmdList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);
		//바로 실행되지는 않는다. GPU에서 때가 되면 명령들을 실행하게 된다.
		//GPU 큐에 명령들을 집어넣는 것.

		mSwapChain->Present();// 백 버퍼(B)와 프론트 버퍼(A) 교체

		// Wait until frame commands are complete.  This waiting is inefficient and is
		// done for simplicity.  Later we will show how to organize our rendering code
		// so we do not have to wait per frame.
		WaitSync();

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
}

void GraphicsCommandQueue::FlushResourceCommandQueue()
{
}
