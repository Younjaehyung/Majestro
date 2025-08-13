#include "pch.h"
#include "RenderManager.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Buffer.h"
#include "RenderSystem.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "LightComponent.h"
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
	mRenderTargetHeap->Initialize();

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
		group.ParticleInfo->CreateDefaultBuffer(PARTICLE_COUNT, sizeof(PatricleParams));
		group.ParticleInfo->CreateSrvView(i, STRUCTURED_INDEX_START, static_cast<uint32>(STRUCTURED_INDEX::SRV_PARTICLE_INDEX), GROUP_COUNT);

		i++;
	}

}

void RenderManager::CreateParticle()
{
	for (ParticleBuffer& group : mParticleBuffer) {
		group.Particle = make_shared<StructuredBuffer>();
		group.Particle->CreateDefaultBuffer(sizeof(PatricleParams), PARTICLE_COUNT);
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
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 0);

	// SwapChain Group
	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainTarget_" + std::to_wstring(i);

			ComPtr<ID3D12Resource> resource;
			mSwapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));	//SwapChainBuffer를 가져옴
			rtVec[i].Target = RESOURCEMANAGER.CreateTextureFromResource(name, resource, 0);	//SwapChainBuffer을 이용해서 Texutre 생성
			//SwapChainBuffer을 이용해서 Texutre 생성
		}


		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)].Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}
	//======공용=======
	// Shadow Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"ShadowTarget",
			DXGI_FORMAT_R32_FLOAT, 4096, 4096,	//32bit R값으로 세팅함
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, false);

		shared_ptr<Texture> shadowDepthTexture = RESOURCEMANAGER.CreateTexture(L"ShadowDepthStencil",
			DXGI_FORMAT_D32_FLOAT, 4096, 4096,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 0);

		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)].Create(RENDER_TARGET_GROUP_TYPE::SHADOW, rtVec, shadowDepthTexture);
	}

	// Deferred Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PositionTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"NormalTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[2].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);


		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)].Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, dsTexture);
	}


	// Lighting Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"SpecularLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,0);


		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)].Create(RENDER_TARGET_GROUP_TYPE::LIGHTING, rtVec, dsTexture);
	}

	int i = 0;
	for (auto& renderTargetGroup : mRenderTargetGroup) {
		
		if (renderTargetGroup.GetGroupType() == RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN) {
			continue;
		}


		for (auto& renderTarget : renderTargetGroup.GetRTG()) {

			
			UINT mipLevels = 1;	// 임시 밈맵

			// Texture에 대한 SRV 서술자 설정
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 기본 매핑 (RGBA -> RGBA)
			srvDesc.Format = renderTarget.Target->GetOriginalImage().GetMetadata().format; // 텍스처의 실제 포맷

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
				srvDesc.Texture2DArray.ArraySize = renderTarget.Target->GetTex2D()->GetDesc().DepthOrArraySize; // 배열 크기
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
			D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = mGraphicsDescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_CPU_DESCRIPTOR_HANDLE srvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuhandle, (i ) * srvSize);
			DEVICE->CreateShaderResourceView(renderTarget.Target->GetTex2D().Get(), &srvDesc, srvhandle);
			renderTarget.Target->SetSrvHandle(srvhandle);
			i++;
		}


	}

}
