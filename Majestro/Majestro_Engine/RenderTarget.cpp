#include "pch.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "RenderManager.h"


void RenderTarget::Initialize(shared_ptr<Texture> dsTexture)
{
	mRenderTargetCount = 6;
	mDepthStencilTexture = dsTexture;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc1{};
	heapDesc1.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc1.NumDescriptors = 5;
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

	//create시 베리어 생성
	for (uint32 i = 0; i < mRenderTargetCount; ++i)
	{
		
	}
}

void RenderTarget::Create(RENDER_TARGET_GROUP_TYPE groupType, RenderTargetStruct& rtStru, uint8 type)
{
	mRenderTargetType = groupType;
	mRenderTargetVec.push_back(rtStru);
	

		uint32 dsvHeapSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		uint32 srvHeapSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = mRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE srvHeapBegin = Graphics_DescHeap->GetDescriptorHeap().Get()->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE Rhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin,  type * mRtvHeapSize);
		D3D12_CPU_DESCRIPTOR_HANDLE Dhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeapBegin, type * dsvHeapSize);
		D3D12_CPU_DESCRIPTOR_HANDLE Shandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHeapBegin, type * srvHeapSize);

		rtStru.Target->SetRtvHandle(Rhandle);

		if (groupType != RENDER_TARGET_GROUP_TYPE::SHADOW) {
			DEVICE->CreateRenderTargetView(rtStru.Target->GetTex2D().Get(), nullptr, Rhandle);
		}
		else {
			DEVICE->CreateDepthStencilView(rtStru.Target->GetTex2D().Get(), nullptr, Dhandle);
			rtStru.Target->SetDsvHandle(Dhandle);
		}

		if (mRenderTargetType != RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN) {
			UINT mipLevels = 1;	// 임시 밈맵

			// Texture에 대한 SRV 서술자 설정
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 기본 매핑 (RGBA -> RGBA)
			srvDesc.Format = rtStru.Target->GetOriginalImage().GetMetadata().format; // 텍스처의 실제 포맷
			
			// ViewDimension에 따라 다른 구조체 필드를 설정
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

			switch (srvDesc.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				srvDesc.Texture2D.MostDetailedMip = 0;       // 가장 높은 해상도의 밉맵부터 시작
				srvDesc.Texture2D.MipLevels = mipLevels;     // 사용할 밉맵 레벨 수
				srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스 (비디오 텍스처 등에서 사용)
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 최소 LOD 클램프
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = mipLevels;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = rtStru.Target->GetTex2D()->GetDesc().DepthOrArraySize; // 배열 크기
				srvDesc.Texture2DArray.PlaneSlice = 0;
				srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = mipLevels;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
				break;
				// 다른 텍스처 차원에 대한 case 추가 (3D, CubeArray 등)
			default:
				// 지원하지 않는 차원에 대한 오류 처리
				break;
			}



			DEVICE->CreateShaderResourceView(rtStru.Target->GetTex2D().Get(),&srvDesc, Shandle);
		}
	


		mTargetToResource[i] = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargetVec[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);	//랜더타켓 용도를 common으로

		mResourceToTarget[i] = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargetVec[i].Target->GetTex2D().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);	//common 용도를 랜더타켓으로

		i++;
}

void RenderTarget::OMSetRenderTargets(uint8 type,uint32 count, uint32 offset)
{
	D3D12_VIEWPORT vp = D3D12_VIEWPORT{ 0.f, 0.f, mRenderTargetVec[0].Target->GetWidth() , mRenderTargetVec[0].Target->GetHeight(), 0.f, 1.f };
	D3D12_RECT rect = D3D12_RECT{ 0, 0, static_cast<LONG>(mRenderTargetVec[0].Target->GetWidth()),  static_cast<LONG>(mRenderTargetVec[0].Target->GetHeight()) };

	GRAPHICS_CMD_LIST->RSSetViewports(1, &vp);
	GRAPHICS_CMD_LIST->RSSetScissorRects(1, &rect);


	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeapBegin, offset * mRtvHeapSize);
	GRAPHICS_CMD_LIST->OMSetRenderTargets(count, &rtvHandle, FALSE/*1개*/, &mDsvHeapBegin);
}

void RenderTarget::OMSetRenderTargets(uint8 type)
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
