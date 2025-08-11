#include "pch.h"
#include "RenderManager.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Buffer.h"
#include "RenderSystem.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "ParticleComponent.h"

void RenderManager::Initialize(const WindowInfo& info)
{
	mWindow = info;

	mViewport = { 0, 0, static_cast<FLOAT>(info.Width), static_cast<FLOAT>(info.Height), 0.0f, 1.0f };	//뷰포트창 세팅
	mScissorRect = CD3DX12_RECT(0, 0, info.Width, info.Height);	//사각형 생성

	mDevice->Initialize();



	mGraphicsCommandQueue->Initialize(mDevice->GetDevice(), mSwapChain);
	//_computeCmdQueue->Init(_device->GetDevice());

	mSwapChain->Initialize(info, mDevice->GetDevice(), mDevice->GetDXGI(), mGraphicsCommandQueue->GetCommandQueue());

	

	mGraphicsDescHeap->Initialize(FRAMEGROUP_COUNT);

	CreateGroup();
	CreateParticle();

	CreateRenderTargetGroups();
}

void RenderManager::CreateGlobal()
{

}

void RenderManager::CreateGroup()
{
	// 추후) 1000은 임의의 큰 고정number임. 게임의 scene을 모두 읽고 총 객체 size로 reset하게 할거임


	uint8 i = 0;
	for (GroupBuffer& group : mGroupBuffer) {
		group.PassInfo = make_shared<ConstantBuffer>();
		group.PassInfo->CreateBuffer(sizeof(PassParams));
		group.PassInfo->CreateView(i,CONSTANT_INDEX_START , static_cast<uint32>(CONSTANT_INDEX::CBV_PASSINFO_INDEX),GROUP_COUNT);

		group.LightInfo = make_shared<StructuredBuffer>();
		group.LightInfo->CreateUploadBuffer(1000, sizeof(LightParams));
		group.LightInfo->CreateSrvView(i,STRUCTURED_INDEX_START , static_cast<uint32>(STRUCTURED_INDEX::SRV_LIGHT_INDEX), GROUP_COUNT);


		group.ObjectInfo = make_shared<StructuredBuffer>();
		group.ObjectInfo->CreateUploadBuffer(5000, sizeof(ObjectParams));
		group.ObjectInfo->CreateSrvView(i, STRUCTURED_INDEX_START, static_cast<uint32>(STRUCTURED_INDEX::SRV_OBJECTINFO_INDEX), GROUP_COUNT);

		group.MaterialInfo = make_shared<StructuredBuffer>();
		group.MaterialInfo->CreateUploadBuffer(1000,sizeof(MaterialParams));
		group.MaterialInfo->CreateSrvView(i, STRUCTURED_INDEX_START, static_cast<uint32>(STRUCTURED_INDEX::SRV_MATERIALS_INDEX), GROUP_COUNT);
		
		group.ParticleInfo = make_shared<StructuredBuffer>();
		group.ParticleInfo->CreateDefaultBuffer(PARTICLE_COUNT, sizeof(ParticleParms));
		group.ParticleInfo->CreateSrvView(i, STRUCTURED_INDEX_START, static_cast<uint32>(STRUCTURED_INDEX::SRV_MATERIALS_INDEX), GROUP_COUNT);

		i++;
	}

}

void RenderManager::CreateParticle()
{
	for (ParticleBuffer& group : mParticleBuffer) {
		group.Particle = make_shared<StructuredBuffer>();
		group.Particle->CreateDefaultBuffer(sizeof(ParticleParms), PARTICLE_COUNT);
		group.Particle->CreateSrvView(0, PARTICLE_INDEX_START,static_cast<uint32>(PARTICLE_INDEX::SRV_PARTICLE_INDEX));
		group.Particle->CreateUavView(0, PARTICLE_INDEX_START,static_cast<uint32>(PARTICLE_INDEX::UAV_PARTICLE_INDEX));
		

		group.RWParticleShared = make_shared<StructuredBuffer>();
		group.RWParticleShared->CreateDefaultBuffer(1, sizeof(uint32));
		group.RWParticleShared->CreateUavView(0, PARTICLE_INDEX_START, static_cast<uint32>(PARTICLE_INDEX::UAV_PARTICLE_SHARED_INDEX));

	}

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

		
		mGBufferTarget->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
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

		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)] = make_shared<RenderTarget>();
		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)]->Create(RENDER_TARGET_GROUP_TYPE::SHADOW, rtVec, shadowDepthTexture);
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

		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)] = make_shared<RenderTarget>();
		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)]->Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, dsTexture);
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

		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)] = make_shared<RenderTarget>();
		mGBufferTarget[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)]->Create(RENDER_TARGET_GROUP_TYPE::LIGHTING, rtVec, dsTexture);
	}



}
