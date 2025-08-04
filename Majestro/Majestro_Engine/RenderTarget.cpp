#include "pch.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "RenderManager.h"


void RenderTarget::Create(RENDER_TARGET_GROUP_TYPE groupType, vector<RenderTargetStruct>& rtVec, shared_ptr<Texture> dsTexture)
{
	mRenderTargetType = groupType;
	mRenderTargetVec = rtVec;
	mRenderTargetCount = static_cast<uint32>(rtVec.size());
	mDepthStencilTexture = dsTexture;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc1 {};
	heapDesc1.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc1.NumDescriptors = mRenderTargetCount;
	heapDesc1.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc1.NodeMask = 0;

	DEVICE->CreateDescriptorHeap(&heapDesc1, IID_PPV_ARGS(&mRenderTargetHeap));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc2 = {};
	heapDesc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc2.NumDescriptors = 1;
	heapDesc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc2.NodeMask = 0;
	DEVICE->CreateDescriptorHeap(&heapDesc2, IID_PPV_ARGS(&mDepthStencilHeap));

	mRtvHeapSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mRtvHeapBegin = mRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
	mDsvHeapBegin = mDepthStencilHeap->GetCPUDescriptorHandleForHeapStart();

	
		DEVICE->CreateDepthStencilView(dsTexture->GetTex2D().Get(), nullptr, mDsvHeapBegin);
		dsTexture->SetDsvHandle(mDsvHeapBegin);

	for (uint32  i = 0; i < mRenderTargetCount; i++)
	{


		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = mRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * mRtvHeapSize);

		rtVec[i].Target->SetRtvHandle(handle);


		DEVICE->CreateRenderTargetView(rtVec[i].Target->GetTex2D().Get(), nullptr, handle);

		
	}

	//create시 베리어 생성
	for (uint32  i = 0; i < mRenderTargetCount; ++i)
	{
		mTargetToResource[i] = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargetVec[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);	//랜더타켓 용도를 common으로

		mResourceToTarget[i] = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargetVec[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);	//common 용도를 랜더타켓으로
	}
}

void RenderTarget::OMSetRenderTargets(uint32 count, uint32 offset)
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargetVec[0].Target->GetWidth() , mRenderTargetVec[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargetVec[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargetVec[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);


	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeapBegin, offset * mRtvHeapSize);
	GRAPHICS_CMD_LIST->OMSetRenderTargets(count, &rtvHandle, FALSE/*1개*/, &mDsvHeapBegin);
}

void RenderTarget::OMSetRenderTargets()
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargetVec[0].Target->GetWidth() , mRenderTargetVec[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargetVec[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargetVec[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	GRAPHICS_CMD_LIST->OMSetRenderTargets(mRenderTargetCount, &mRtvHeapBegin, TRUE/*다중*/, &mDsvHeapBegin);
}

void RenderTarget::ClearRenderTargetView(uint32 index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeapBegin, index * mRtvHeapSize);
	GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargetVec[index].ClearColor, 0, nullptr);

	//DepthStencil관련 초기화
	GRAPHICS_CMD_LIST->ClearDepthStencilView(mDsvHeapBegin, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
}

void RenderTarget::ClearRenderTargetView()
{
	WaitResourceToTarget();	//클리어 하기전에 리소스를 타켓으로 변환

	for (uint32 i = 0; i < mRenderTargetCount; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeapBegin, i * mRtvHeapSize);
		GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargetVec[i].ClearColor, 0, nullptr);
	}

	GRAPHICS_CMD_LIST->ClearDepthStencilView(mDsvHeapBegin, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
}

void RenderTarget::WaitTargetToResource()
{
	GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mTargetToResource);
}

void RenderTarget::WaitResourceToTarget()
{
	GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mResourceToTarget);
}
