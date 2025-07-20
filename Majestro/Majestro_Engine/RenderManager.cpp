#include "pch.h"
#include "RenderManager.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Buffer.h"



void RenderManager::Initialize(const WindowInfo& info)
{
	mWindow = info;

	mViewport = { 0, 0, static_cast<FLOAT>(info.Width), static_cast<FLOAT>(info.Height), 0.0f, 1.0f };	//뷰포트창 세팅
	mScissorRect = CD3DX12_RECT(0, 0, info.Width, info.Height);	//사각형 생성

	mDevice->Initialize();



	mGraphicsCommandQueue->Initialize(mDevice->GetDevice(), mSwapChain);
	//_computeCmdQueue->Init(_device->GetDevice());

	mSwapChain->Initialize(info, mDevice->GetDevice(), mDevice->GetDXGI(), mGraphicsCommandQueue->GetCommandQueue());



	mGraphicsDescHeap->Initialize(256);
	//_computeDescHeap->Initialize();

	CreateConstantBuffer(CBV_REGISTER::b0, sizeof(LightParams), 1);	//LightParams(50) 정보 넘김
	CreateConstantBuffer(CBV_REGISTER::b1, sizeof(CameraParams), 1);	//LightParams(50) 정보 넘김
	//CreateConstantBuffer(CBV_REGISTER::b1, sizeof(TransformParams), 256);	//TransformParams 256개 생성
	CreateConstantBuffer(CBV_REGISTER::b2, sizeof(MaterialParams), 256);	//MaterialParams 256개 생성


	CreateRenderTargetGroups();
}

void RenderManager::Update()
{
}

void RenderManager::StartRender()
{
	mGraphicsCommandQueue->RenderBegin();
}


void RenderManager::EndRender()
{
	mGraphicsCommandQueue->RenderEnd();
}

void RenderManager::ResizeWindow(int32 width, int32 height)
{
	mWindow.Width = width;
	mWindow.Height = height;

	//윈도우 창 사이즈 조절
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(mWindow.Hwnd, 0, 100, 100, width, height, 0);

}



void RenderManager::CreateRenderTargetGroups()
{
	// DepthStencil
	shared_ptr<Texture> dsTexture = gEngine->GetResourceManager().CreateTexture(L"DepthStencil",
		DXGI_FORMAT_D32_FLOAT, mWindow.Width, mWindow.Height,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// SwapChain Group
	{
		vector<RenderTargetStruct> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainTarget_" + std::to_wstring(i);

			ComPtr<ID3D12Resource> resource;
			mSwapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));	//SwapChainBuffer를 가져옴
			rtVec[i].Target = RESOURCEMANAGER.CreateTextureFromResource(name, resource);	//SwapChainBuffer을 이용해서 Texutre 생성
				//SwapChainBuffer을 이용해서 Texutre 생성
		}

		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)] = make_shared<RenderTarget>();
		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}
	//======공용=======
	// Shadow Group
	{
		vector<RenderTargetStruct> rtVec(RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"ShadowTarget",
			DXGI_FORMAT_R32_FLOAT, 4096, 4096,	//32bit R값으로 세팅함
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		shared_ptr<Texture> shadowDepthTexture = RESOURCEMANAGER.CreateTexture(L"ShadowDepthStencil",
			DXGI_FORMAT_D32_FLOAT, 4096, 4096,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)] = make_shared<RenderTarget>();
		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)]->Create(RENDER_TARGET_GROUP_TYPE::SHADOW, rtVec, shadowDepthTexture);
	}

	// Deferred Group
	{
		vector<RenderTargetStruct> rtVec(RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PositionTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"NormalTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[2].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)] = make_shared<RenderTarget>();
		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)]->Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, dsTexture);
	}


	// Lighting Group
	{
		vector<RenderTargetStruct> rtVec(RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"SpecularLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)] = make_shared<RenderTarget>();
		mRenderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)]->Create(RENDER_TARGET_GROUP_TYPE::LIGHTING, rtVec, dsTexture);
	}
}

void RenderManager::CreateConstantBuffer(CBV_REGISTER reg, uint32 bufferSize, uint32 count)
{
	uint8 typeInt = static_cast<uint8>(reg);
	assert(mConstantBuffer.size() == typeInt);

	shared_ptr<ConstantBuffer> buffer = make_shared<ConstantBuffer>();
	buffer->Initialize(reg, bufferSize, count);
	mConstantBuffer.push_back(buffer);
}
