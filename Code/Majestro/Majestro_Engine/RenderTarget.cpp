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
	// uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32 rtvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	
	// D3D12_CPU_DESCRIPTOR_HANDLE srvHeapBegin = Graphics_DescHeap->GetDescriptorHeap().Get()->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = RENDERMANAGER.GetRenderTargetHeap()->GetRtvHeapBegin();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapBegin = RENDERMANAGER.GetRenderTargetHeap()->GetDsvHeapBegin();

	uint32 rtvIndex = RENDERMANAGER.GetRenderTargetHeap()->GetRtvIndex();
	uint32 dsvIndex = RENDERMANAGER.GetRenderTargetHeap()->GetDsvIndex();

	mRTHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, (rtvIndex * rtvSize));
	mDSHeapBegin = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, (dsvIndex * dsvSize));

	mSliceRTVHandles.clear();
	mSliceDSVHandles.clear();

	for (auto& target: rtStru) {
		

		D3D12_CPU_DESCRIPTOR_HANDLE rtvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, (rtvIndex * rtvSize));
		D3D12_CPU_DESCRIPTOR_HANDLE dsvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, (dsvIndex * dsvSize));


		if (groupType != RENDER_TARGET_GROUP_TYPE::SHADOW && groupType != RENDER_TARGET_GROUP_TYPE::PRE_DEPTH)
		{
			DEVICE->CreateRenderTargetView(target.Target->GetTex2D().Get(), nullptr, rtvhandle);
		}


		mDepthStencilTexture = dsTexture;
		if (mDepthStencilTexture) {
			if (groupType == RENDER_TARGET_GROUP_TYPE::SHADOW && mDepthStencilTexture->GetTex2D()->GetDesc().DepthOrArraySize > 1)
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.Texture2DArray.MipSlice = 0;
				dsvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(mSliceDSVHandles.size());
				dsvDesc.Texture2DArray.ArraySize = 1;
				DEVICE->CreateDepthStencilView(dsTexture->GetTex2D().Get(), &dsvDesc, dsvhandle);
				mSliceDSVHandles.push_back(dsvhandle);
			}
			else
			{
				// R32_TYPELESS 텍스처는 nullptr desc 불가 → D32_FLOAT로 명시
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.Texture2D.MipSlice = 0;
				DEVICE->CreateDepthStencilView(dsTexture->GetTex2D().Get(), &dsvDesc, dsvhandle);
			}
			RENDERMANAGER.GetRenderTargetHeap()->SetDsvIndex(++dsvIndex);
		}

		RENDERMANAGER.GetRenderTargetHeap()->SetRtvIndex(++rtvIndex);
		
	}

	// PRE_DEPTH: rtStru가 비어있어도 DSV는 반드시 생성해야 함
	if (rtStru.empty() && dsTexture)
	{
		mDepthStencilTexture = dsTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, dsvIndex * dsvSize);
		mDSHeapBegin = dsvhandle;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;
		DEVICE->CreateDepthStencilView(dsTexture->GetTex2D().Get(), &dsvDesc, dsvhandle);
		RENDERMANAGER.GetRenderTargetHeap()->SetDsvIndex(++dsvIndex);
	}

	for (uint32 i = 0; i < rtStru.size(); ++i) {
		mRenderTargets.push_back(rtStru[i]);
	}

	// ─── Read-only DSV 생성 (Rim Light: DEPTH_READ | PIXEL_SHADER_RESOURCE 동시 접근용) ───
	// SHADOW는 Texture2DArray slice DSV를 사용하므로 제외
	// ForwardPass에서 Gbuffer[0]을 SRV로 읽으면서 depth test도 수행하기 위함
	if (mDepthStencilTexture && mSliceDSVHandles.empty())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE roDsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeapBegin, dsvIndex * dsvSize);
		mDSHeapBeginReadOnly = roDsvHandle;

		D3D12_DEPTH_STENCIL_VIEW_DESC roDsvDesc = {};
		roDsvDesc.Format                 = DXGI_FORMAT_D32_FLOAT;
		roDsvDesc.ViewDimension          = D3D12_DSV_DIMENSION_TEXTURE2D;
		roDsvDesc.Flags                  = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		roDsvDesc.Texture2D.MipSlice     = 0;
		DEVICE->CreateDepthStencilView(mDepthStencilTexture->GetTex2D().Get(), &roDsvDesc, roDsvHandle);
		RENDERMANAGER.GetRenderTargetHeap()->SetDsvIndex(++dsvIndex);
	}

	//create시 베리어 생성
	//for (uint32 i = 0; i < rtStru.size(); ++i)
	//{
	//	if (groupType == RENDER_TARGET_GROUP_TYPE::SHADOW && mDepthStencilTexture)
	//	{
	//		mTargetToResource[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilTexture->GetTex2D().Get(),
	//			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	//		mResourceToTarget[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilTexture->GetTex2D().Get(),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//	}
	//	else
	//	{
	//		mTargetToResource[i] = CD3DX12_RESOURCE_BARRIER::Transition(rtStru[i].Target->GetTex2D().Get(),
	//			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

	//		mResourceToTarget[i] = CD3DX12_RESOURCE_BARRIER::Transition(rtStru[i].Target->GetTex2D().Get(),
	//			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//	}

	//}

	
	mTargetToResource.reserve(mRenderTargets.size());
	// unordered_set<ID3D12Resource*> visited;

	if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW && mDepthStencilTexture)
	{
		ID3D12Resource* resource = mDepthStencilTexture->GetTex2D().Get();
		mTargetToResource.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
	else {
		for (auto& rt : mRenderTargets)
		{
			ID3D12Resource* resource = rt.Target->GetTex2D().Get();
				mTargetToResource.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON));
		}
	}



	mResourceToTarget.reserve(mRenderTargets.size());

	if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW && mDepthStencilTexture)
	{
		ID3D12Resource* resource = mDepthStencilTexture->GetTex2D().Get();
		mResourceToTarget.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}
	else {
		for (auto& rt : mRenderTargets)
		{
			ID3D12Resource* resource = rt.Target->GetTex2D().Get();
				mResourceToTarget.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource,
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
	}

}

void RenderTargetGroup::OMSetRenderTargets(uint32 count, uint32 offset)
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargets[0].Target->GetWidth() , mRenderTargets[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargets[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargets[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	if (!mSliceRTVHandles.empty() && offset < mSliceRTVHandles.size())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mSliceRTVHandles[offset];
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = (!mSliceDSVHandles.empty() && offset < mSliceDSVHandles.size()) ? mSliceDSVHandles[offset] : mDSHeapBegin;
		if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW)
		{
			GRAPHICS_CMD_LIST->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		}
		else
		{
			GRAPHICS_CMD_LIST->OMSetRenderTargets(count, &rtvHandle, FALSE/*once*/, &dsvHandle);
		}
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, offset * size);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = (!mSliceDSVHandles.empty() && offset < mSliceDSVHandles.size()) ? mSliceDSVHandles[offset] : mDSHeapBegin;
	if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW)
		GRAPHICS_CMD_LIST->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
	else
		GRAPHICS_CMD_LIST->OMSetRenderTargets(count, &rtvHandle, FALSE/*once*/, &dsvHandle);
}

void RenderTargetGroup::OMSetRenderTargets()
{
	float width  = 0.f;
	float height = 0.f;

	if (!mRenderTargets.empty())
	{
		width  = static_cast<float>(mRenderTargets[0].Target->GetWidth());
		height = static_cast<float>(mRenderTargets[0].Target->GetHeight());
	}
	else if (mDepthStencilTexture)
	{
		// PRE_DEPTH처럼 컬러 RT 없이 Depth만 있는 경우
		width  = static_cast<float>(mDepthStencilTexture->GetWidth());
		height = static_cast<float>(mDepthStencilTexture->GetHeight());
	}

	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, width, height, 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW || mGroupType == RENDER_TARGET_GROUP_TYPE::PRE_DEPTH)
		GRAPHICS_CMD_LIST->OMSetRenderTargets(0, nullptr, FALSE, &mDSHeapBegin);
	else
		GRAPHICS_CMD_LIST->OMSetRenderTargets(mRenderTargetCount, &mRTHeapBegin, TRUE/*multi*/, &mDSHeapBegin);
}

void RenderTargetGroup::ClearRenderTargetView(uint32 index)
{
	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	if (!mSliceRTVHandles.empty() && index < mSliceRTVHandles.size())
		rtvHandle = mSliceRTVHandles[index];
	else
		rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, index * size);

	if (mGroupType != RENDER_TARGET_GROUP_TYPE::SHADOW)
		GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargets[std::min<uint32>(index, mRenderTargetCount - 1)].ClearColor, 0, nullptr);


	//DepthStencil관련 초기화
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = (!mSliceDSVHandles.empty() && index < mSliceDSVHandles.size()) ? mSliceDSVHandles[index] : mDSHeapBegin;
	GRAPHICS_CMD_LIST->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

}

void RenderTargetGroup::TransitionToTarget()
{
	// SHADOW, PRE_DEPTH: 텍스처 초기 상태가 DEPTH_WRITE이므로 첫 프레임엔 배리어 불필요
	if ((mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW || mGroupType == RENDER_TARGET_GROUP_TYPE::PRE_DEPTH) && mFirstUse)
	{
		mFirstUse = false;
	}
	else
	{
		WaitResourceToTarget();	//그리기 전에 리소스를 타켓으로 변환
	}
}

void RenderTargetGroup::ClearRenderTargetView()
{
	TransitionToTarget();
	uint32 size = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (mGroupType == RENDER_TARGET_GROUP_TYPE::SHADOW)
	{
		if (!mSliceRTVHandles.empty())
		{
			for (uint32 i = 0; i < mSliceRTVHandles.size(); ++i)
				GRAPHICS_CMD_LIST->ClearRenderTargetView(mSliceRTVHandles[i], mRenderTargets[0].ClearColor, 0, nullptr);

		}
	}
	else
	{
		for (uint32 i = 0; i < mRenderTargetCount; i++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTHeapBegin, i * size);
			GRAPHICS_CMD_LIST->ClearRenderTargetView(rtvHandle, mRenderTargets[i].ClearColor, 0, nullptr);
		}
	}

	
	if (!mSliceDSVHandles.empty())
	{
		for (auto& dsvHandle : mSliceDSVHandles)
			GRAPHICS_CMD_LIST->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}
	else
	{
		GRAPHICS_CMD_LIST->ClearDepthStencilView(mDSHeapBegin, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}
}

void RenderTargetGroup::OMSetRenderTargetsReadOnlyDepth()
{
	float width  = 0.f;
	float height = 0.f;

	if (!mRenderTargets.empty())
	{
		width  = static_cast<float>(mRenderTargets[0].Target->GetWidth());
		height = static_cast<float>(mRenderTargets[0].Target->GetHeight());
	}
	else if (mDepthStencilTexture)
	{
		width  = static_cast<float>(mDepthStencilTexture->GetWidth());
		height = static_cast<float>(mDepthStencilTexture->GetHeight());
	}

	D3D12_VIEWPORT vp   = D3D12_VIEWPORT{ 0.f, 0.f, width, height, 0.f, 1.f };
	D3D12_RECT     rect = D3D12_RECT{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);

	// read-only DSV 사용: DEPTH_READ 상태에서 depth test 가능, depth write 불가
	if (mDSHeapBeginReadOnly.ptr != 0)
		GRAPHICS_CMD_LIST->OMSetRenderTargets(mRenderTargetCount, &mRTHeapBegin, TRUE, &mDSHeapBeginReadOnly);
	else
		GRAPHICS_CMD_LIST->OMSetRenderTargets(mRenderTargetCount, &mRTHeapBegin, TRUE, nullptr);
}

void RenderTargetGroup::WaitTargetToResource()
{
	//GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mTargetToResource);

	

	if (!mTargetToResource.empty())
		GRAPHICS_CMD_LIST->ResourceBarrier(static_cast<UINT>(mTargetToResource.size()), mTargetToResource.data());
}



void RenderTargetGroup::WaitResourceToTarget()
{
	//GRAPHICS_CMD_LIST->ResourceBarrier(mRenderTargetCount, mResourceToTarget);
	
	
	if (!mResourceToTarget.empty())
		GRAPHICS_CMD_LIST->ResourceBarrier(static_cast<UINT>(mResourceToTarget.size()), mResourceToTarget.data());
}
