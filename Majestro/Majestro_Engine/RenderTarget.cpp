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

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = mRenderTargetCount;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	DEVICE->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mRenderTargetHeap));

	mRtvHeapSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mRtvHeapBegin = mRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
	mDsvHeapBegin = mDepthStencilTexture->GetDSV()->GetCPUDescriptorHandleForHeapStart();

	for (uint32  i = 0; i < mRenderTargetCount; i++)
	{
		uint32 destSize = 1;
		D3D12_CPU_DESCRIPTOR_HANDLE destHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeapBegin, i * mRtvHeapSize);

		uint32 srcSize = 1;
		ComPtr<ID3D12DescriptorHeap> srcRtvHeapBegin = mRenderTargetVec[i].Target->GetRTV();
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = srcRtvHeapBegin->GetCPUDescriptorHandleForHeapStart();

		DEVICE->CopyDescriptors(1, &destHandle, &destSize, 1, &srcHandle, &srcSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
