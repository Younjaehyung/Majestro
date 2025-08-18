#include "pch.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "RenderManager.h"


void RenderTargetHeap::Initialize()
{

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc1{};
	heapDesc1.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc1.NumDescriptors = 100;
	heapDesc1.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc1.NodeMask = 0;
	DEVICE->CreateDescriptorHeap(&heapDesc1, IID_PPV_ARGS(&mRenderTargetHeap));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc2 = {};
	heapDesc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc2.NumDescriptors = 100;
	heapDesc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc2.NodeMask = 0;
	DEVICE->CreateDescriptorHeap(&heapDesc2, IID_PPV_ARGS(&mDepthStencilHeap));


	mRtvHeapBegin = mRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
	mDsvHeapBegin = mDepthStencilHeap->GetCPUDescriptorHandleForHeapStart();

}

void RenderTargetGroup::Create(RENDER_TARGET_GROUP_TYPE groupType, vector<RenderTarget>& rtStru, shared_ptr<Texture> dsTexture)
{
	
	mGroupType = groupType;
	mRenderTargetCount = static_cast<uint32>(rtStru.size());

	uint32 dsvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32 rtvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	
	D3D12_CPU_DESCRIPTOR_HANDLE srvHeapBegin = Graphics_DescHeap->GetDescriptorHeap().Get()->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = RENDERMANAGER.GetRenderTargetHeap()->GetRtvHeapBegin();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapBegin = RENDERMANAGER.GetRenderTargetHeap()->GetDsvHeapBegin();

	uint32 rtvIndex = RENDERMANAGER.GetRenderTargetHeap()->GetRtvIndex();
	uint32 dsvIndex = RENDERMANAGER.GetRenderTargetHeap()->GetDsvIndex();

	mRTHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, (rtvIndex * rtvSize));
	mDSHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, (dsvIndex * dsvSize));




	for (auto& target: rtStru) {
		

		D3D12_CPU_DESCRIPTOR_HANDLE rtvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, (rtvIndex * rtvSize));
		D3D12_CPU_DESCRIPTOR_HANDLE dsvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, (dsvIndex * dsvSize));


		DEVICE->CreateRenderTargetView(target.Target->GetTex2D().Get(), nullptr, rtvhandle);
		target.Target->SetRtvHandle(rtvhandle);
		target.mHeapBegin=rtvhandle;


		mDepthStencilTexture = dsTexture;
		if (mDepthStencilTexture) {
			DEVICE->CreateDepthStencilView(dsTexture->GetTex2D().Get(), nullptr, dsvhandle);
			RENDERMANAGER.GetRenderTargetHeap()->SetDsvIndex(++dsvIndex);
		}

		RENDERMANAGER.GetRenderTargetHeap()->SetRtvIndex(++rtvIndex);
		
	}

	//mRenderTargets.insert(mRenderTargets.end(), rtStru.begin(), rtStru.end());
	for (uint32 i = 0; i < rtStru.size(); ++i) {
		mRenderTargets.push_back(rtStru[i]);
	}


	//create시 베리어 생성
	for (uint32 i = 0; i < rtStru.size(); ++i)
	{
		mTargetToResource[i] = CD3DX12_RESOURCE_BARRIER::Transition(rtStru[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);	//랜더타켓 용도를 common으로

		mResourceToTarget[i] = CD3DX12_RESOURCE_BARRIER::Transition(rtStru[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);	//common 용도를 랜더타켓으로

	}


}

void RenderTargetGroup::OMSetRenderTargets(uint32 count, uint32 offset)
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargets[0].Target->GetWidth() , mRenderTargets[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargets[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargets[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, offset * size);
	GRAPHICS_CMD_LIST->OMSetRenderTargets(count, &rtvHandle, FALSE/*once*/, &mDSHeapBegin);
}

void RenderTargetGroup::OMSetRenderTargets()
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargets[0].Target->GetWidth() , mRenderTargets[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargets[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargets[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	GRAPHICS_CMD_LIST->OMSetRenderTargets(mRenderTargetCount, &mRTHeapBegin, TRUE/*multi*/, &mDSHeapBegin);

}

void RenderTargetGroup::ClearRenderTargetView(uint32 index)
{
	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, index * size);
	GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargets[index].ClearColor, 0, nullptr);

	//DepthStencil관련 초기화
	GRAPHICS_CMD_LIST->ClearDepthStencilView(mDSHeapBegin, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
}

void RenderTargetGroup::ClearRenderTargetView()
{
	WaitResourceToTarget();	//클리어 하기전에 리소스를 타켓으로 변환
	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (uint32 i = 0; i < mRenderTargetCount; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, i * size);
		GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargets[i].ClearColor, 0, nullptr);
	}

	GRAPHICS_CMD_LIST->ClearDepthStencilView(mDSHeapBegin, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
}

void RenderTargetGroup::WaitTargetToResource()
{
	GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mTargetToResource);
}

void RenderTargetGroup::WaitResourceToTarget()
{
	GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mResourceToTarget);
}
