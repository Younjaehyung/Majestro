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
#include "AnimationComponent.h"
#include "Animator.h"
#include "UIRenderSystem.h"
#include "Skeleton.h"
void RenderManager::Initialize(const WindowInfo& info)
{
	mWindow = info;

	mFrameResourceIndex = 0;
	mFrameCurrIndex = 0;
	// TO - DO : 임시로 4K 해상도로 고정
	//mWindow.Width = 2560;
	//mWindow.Height = 1440;



	mViewport = { 0, 0, static_cast<FLOAT>(mWindow.Width), static_cast<FLOAT>(mWindow.Height), 0.0f, 1.0f };	// 뷰포트 창 세팅
	mScissorRect = CD3DX12_RECT(0, 0, mWindow.Width, mWindow.Height);	// 사각형 생성

	mDevice->Initialize();

	CheckMsaaSupport(DXGI_FORMAT_R8G8B8A8_UNORM, 4);


	mGraphicsCommandQueue->Initialize(mDevice->GetDevice(), mSwapChain);
	mComputeCommandQueue->Initialize(mDevice->GetDevice());

	mSwapChain->Initialize(mWindow, mDevice->GetDevice(), mDevice->GetDXGI(), mGraphicsCommandQueue->GetCommandQueue());

	


	mDescHeap->Initialize(FRAMEGROUP_COUNT);
	mRenderTargetHeap->Initialize();

	mGraphicsMemory = std::make_shared<DirectX::DX12::GraphicsMemory>(mDevice->GetDevice().Get());


	mParticleDescriptorAllocator.Initialize(
		PARTICLE_INDEX_START,
		static_cast<uint32>(PARTICLE_INDEX::PARTICLE_INDEX_END),
		PARTICLE_RUNTIME_POOL_COUNT);

	mParticleResourceAllocator.Initialize();

	CreateGroup();
	CreateMaterial();
	CreateAnimation();
	CreateParticle();

	CreateRenderTargetGroups();
	InitEffekseer();
}

void RenderManager::InitEffekseer(int32_t instanceMax, int32_t squareMaxCount)
{
	mEfkGraphicsDevice = EffekseerRendererDX12::CreateGraphicsDevice(
		DEVICE.Get(),
		mGraphicsCommandQueue->GetCommandQueue().Get(),
		SWAP_CHAIN_BUFFER_COUNT);

	// 인게임 VFX용: HDR RT 포맷(R16G16B16A16_FLOAT). ToneMap 전에 렌더링한다.
	DXGI_FORMAT hdrFormats[1] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
	mEfkRendererHDR = EffekseerRendererDX12::Create(
		mEfkGraphicsDevice,
		hdrFormats, 1,
		DXGI_FORMAT_D32_FLOAT,
		false,
		squareMaxCount);

	mEfkMemoryPoolHDR = EffekseerRenderer::CreateSingleFrameMemoryPool(mEfkGraphicsDevice);
	mEfkCmdListHDR    = EffekseerRenderer::CreateCommandList(mEfkGraphicsDevice, mEfkMemoryPoolHDR);

	// UI VFX용: SwapChain 포맷(R8G8B8A8_UNORM). UI 위에 렌더링한다.
	DXGI_FORMAT uiFormats[1] = { DXGI_FORMAT_R8G8B8A8_UNORM };
	mEfkRendererUI = EffekseerRendererDX12::Create(
		mEfkGraphicsDevice,
		uiFormats, 1,
		DXGI_FORMAT_D32_FLOAT,
		false,
		squareMaxCount);

	mEfkMemoryPoolUI = EffekseerRenderer::CreateSingleFrameMemoryPool(mEfkGraphicsDevice);
	mEfkCmdListUI    = EffekseerRenderer::CreateCommandList(mEfkGraphicsDevice, mEfkMemoryPoolUI);
}

void RenderManager::CreateGlobal()
{

}

void RenderManager::CreateGroup()
{
	// 추후) 1000은 임의의 큰 고정 number다. 게임 scene을 모두 읽고 총 객체 size로 reset하게 할 예정이다.

	uint32 i = 0;
	for (shared_ptr<GroupBuffer>& group : mGroupBuffer) {
		group = make_shared<GroupBuffer>();

		group->PassInfo = make_shared<ConstantBuffer>();
		group->PassInfo->CreateBuffer(sizeof(PassParams));
		group->PassInfo->CreateView(i, CONSTANT_INDEX_START, static_cast<uint32>(CONSTANT_INDEX::CBV_PASSINFO_INDEX), GROUP_COUNT);

		group->InstanceInfo = make_shared<StructuredBuffer>();
		// 셰이더가 반복해서 읽는 데이터는 GPU 로컬 메모리에 배치한다.
		// CPU 데이터는 프레임별 업로드 스테이징 버퍼를 거쳐 복사한다.
		group->InstanceInfo->CreateDefaultBuffer(sizeof(RenderParams), 4096 * 40);
		group->InstanceInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_INSTANCE_INDEX), GROUP_COUNT);


		group->ObjectInfo = make_shared<StructuredBuffer>();
		// 여러 셰이더 단계에서 사용하는 오브젝트 정보는 GPU 로컬 메모리에 둔다.
		group->ObjectInfo->CreateDefaultBuffer(sizeof(ObjectParams), 4096 * 40);
		group->ObjectInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_OBJECTINFO_INDEX), GROUP_COUNT);


		group->ParticleInfo = make_shared<StructuredBuffer>();

		group->ParticleInfo->CreateDefaultBuffer(sizeof(ParticleSharedParams), PARTICLE_EMITTER_COUNT);
		group->ParticleInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_PARTICLE_INDEX), GROUP_COUNT);


		group->UIInfo = make_shared<StructuredBuffer>();
		group->UIInfo->CreateUploadBuffer(sizeof(UIInstanceData), UI_SRV_COUNT);
		group->UIInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_UI_INDEX), GROUP_COUNT);

		group->AnimInstanceInfo = make_shared<StructuredBuffer>();
		group->AnimInstanceInfo->CreateUploadBuffer(sizeof(AnimationInstance), 512);
		group->AnimInstanceInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_ANIMINSTANCE_INDEX), GROUP_COUNT);

		group->AnimResultInfo = make_shared<StructuredBuffer>();
		group->AnimResultInfo->CreateDefaultBuffer(sizeof(Matrix), 1024*1000);
		group->AnimResultInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_FINALUBONE_INDEX), GROUP_COUNT);
		group->AnimResultInfo->CreateUavView(i, GROUP_UAV_START, static_cast<uint32>(GROUP_UAV_INDEX::UAV_FINALUBONE_INDEX), GROUP_COUNT);

		// pass별 커스텀 텍스처 및 파라미터 테이블이다. 각 pass가 자신의 행(PASS_CUSTOM_INDEX)을 사용한다.
		group->PassCustomTableInfo = make_shared<StructuredBuffer>();
		// 전체 화면 패스에서 픽셀마다 읽으므로 GPU 로컬 메모리를 사용한다.
		group->PassCustomTableInfo->CreateDefaultBuffer(sizeof(PassCustomData),
			static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT));
		group->PassCustomTableInfo->CreateSrvView(i, GROUP_SRV_START, static_cast<uint32>(GROUP_SRV_INDEX::SRV_PASS_CUSTOM_INDEX), GROUP_COUNT);

		i++;
	}

}

void RenderManager::CreateMaterial()
{
	mMaterialBuffer = make_shared<StructuredBuffer>();
	mMaterialBuffer->CreateDefaultBuffer(sizeof(MaterialParams), 2048);
	mMaterialBuffer->CreateSrvView(0, TEXTURE_MATERIALS_INDEX_START, static_cast<uint32>(TEXTURE_INDEX::TEXTURE_MATERIALS_INDEX));
}

void RenderManager::CreateParticle()
{	
	for (uint32 i = 0; i < FRAMEGROUP_COUNT; i++) {
		mParticleSpawnBuffer[i] = make_shared<StructuredBuffer>();
		mParticleSpawnBuffer[i]->CreateDefaultBuffer(sizeof(ParticleComputeSharedParams), PARTICLE_EMITTER_COUNT);

		std::vector<ParticleComputeSharedParams> initialCounters(PARTICLE_EMITTER_COUNT);
		mParticleSpawnBuffer[i]->PushDefaultToData(
			initialCounters.data(),
			static_cast<uint32>(initialCounters.size() * sizeof(ParticleComputeSharedParams)));

		mParticleSpawnBuffer[i]->CreateUavViewAtIndex(PARTICLE_SPAWN_INDEX_START + i);
		
	}

}

void RenderManager::CreateAnimation()
{
	mAnimationBuffer = make_shared<AnimationBuffer>();

	mAnimationBuffer->SkeletonBone = make_shared<StructuredBuffer>();
	mAnimationBuffer->SkeletonBone->CreateDefaultBuffer(sizeof(SkeletonBoneParams), 128 * 10 * 5);
	mAnimationBuffer->SkeletonBone->CreateSrvView(0, ANIMATION_INDEX_START, static_cast<uint32>(ANIMATION_INDEX::SRV_SKELETONBONE_INDEX));

	mAnimationBuffer->AnimationClip = make_shared<StructuredBuffer>();
	mAnimationBuffer->AnimationClip->CreateDefaultBuffer(sizeof(KeyFrameInfo), 1);
	mAnimationBuffer->AnimationClip->CreateSrvView(0, ANIMATION_INDEX_START, static_cast<uint32>(ANIMATION_INDEX::SRV_ANIMATIONCLIP_INDEX));

	mAnimationBuffer->AnimationMeta = make_shared<StructuredBuffer>();
	mAnimationBuffer->AnimationMeta->CreateDefaultBuffer(sizeof(AnimationClipMeta), 128);
	mAnimationBuffer->AnimationMeta->CreateSrvView(0, ANIMATION_INDEX_START, static_cast<uint32>(ANIMATION_INDEX::SRV_ANIMATIONMETA_INDEX));

}

void RenderManager::Update()
{
	mParticleDescriptorAllocator.GarbageCollect(GetCompletedGraphicsFenceValue());
	mParticleResourceAllocator.GarbageCollect(GetCompletedGraphicsFenceValue());
}

void RenderManager::StartRender()
{
	mSwapChain->UpdateBackBufferIndex();
	const uint32 backIndex = mSwapChain->GetBackBufferIndex();
	mGraphicsCommandQueue->WaitForBackBuffer(backIndex);
	mGraphicsCommandQueue->WaitForFrame(mFrameResourceIndex);
	mGraphicsCommandQueue->RenderBegin(mFrameResourceIndex);


	// 수정: Effekseer command list는 각 pass의 draw 구간에서만 연다.
	// 프레임 전체에서 BeginCommandList를 유지하면 SubmitIndependentWork의 GRAPHICS_CMD_LIST reset과 충돌할 수 있다.
}

void RenderManager::SubmitIndependentGraphics()
{
	// Execute depth, shadow, and deferred work before waiting for Forward Plus compute.
	mGraphicsCommandQueue->SubmitIndependentWork(mFrameResourceIndex);

	// Insert the queue wait after independent work so the graphics and compute queues can overlap.
	mGraphicsCommandQueue->WaitForFence(
		mComputeCommandQueue->GetFence().Get(),
		mAnimationComputeFenceValue);

	// Reset clears command list state, so restore the shared graphics bindings.
	SetGraphicsTable();
}


void RenderManager::EndRender()
{
	const uint32 backIndex = mSwapChain->GetBackBufferIndex();

	// 커맨드 리스트를 먼저 제출한다. ExecuteCommandLists와 Present를 포함한다.
	mGraphicsCommandQueue->RenderEnd();

	mGraphicsMemory->Commit(mGraphicsCommandQueue->GetCommandQueue().Get());

	// Effekseer 내부 fence 신호는 각 pass의 EndCommandList에서 처리한다.
	// 여기서는 엔진 frame fence만 진행한다.
	mGraphicsCommandQueue->SignalFrame(mFrameResourceIndex, backIndex);

	mFrameCurrIndex = mFrameResourceIndex;
	mFrameResourceIndex = (mFrameResourceIndex + 1) % FRAMEGROUP_COUNT;
}

void RenderManager::CheckMsaaSupport(DXGI_FORMAT format, uint32 sampleCount)
{
	mMsaaSampleCount = 1;
	mMsaaQuality = 0;

	if (sampleCount <= 1)
	{
		return;
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels{};
	qualityLevels.Format = format;
	qualityLevels.SampleCount = sampleCount;
	qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	qualityLevels.NumQualityLevels = 0;

	if (SUCCEEDED(mDevice->GetDevice()->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&qualityLevels,
		sizeof(qualityLevels)))
		&& qualityLevels.NumQualityLevels > 0)
	{
		mMsaaSampleCount = sampleCount;
		mMsaaQuality = qualityLevels.NumQualityLevels - 1;
	}
}

void RenderManager::ResizeWindow(int32 width, int32 height)
{
	mWindow.Width = width;
	mWindow.Height = height;

	// 윈도우 창 사이즈 조절
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(mWindow.Hwnd, 0, 100, 100, width, height, 0);

}

void RenderManager::SetComputTable()
{

	if (mRootSignature == nullptr) {
		mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");
	}


	COMPUTE_CMD_LIST->SetComputeRootSignature(mRootSignature->GetRootSignature().Get());	// 루트 시그니처 set

	uint32 mFrameCount = RENDERMANAGER.GetFrameResourceIndex();


	ID3D12DescriptorHeap* descHeap = mDescHeap->GetDescriptorHeap().Get();
	COMPUTE_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	// 사용할 테이블 힙을 선택한다. 무거운 호출이므로 프레임당 1번 사용을 권장한다.
	//COMPUTE_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	// 사용할 테이블 힙을 선택한다. 무거운 호출이므로 프레임당 1번 사용을 권장한다.

	// Table 바인딩
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 1, GBUFFER_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 2, GROUP_START, GROUP_COUNT);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 3, PARTICLE_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 4, PARTICLE_SPAWN_INDEX_START, 1);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 5, ANIMATION_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitComputeTable(mFrameCount, 6, TEXTURE_MATERIALS_INDEX_START);
}

void RenderManager::SetGraphicsTable()
{
	if (mRootSignature == nullptr) {
		mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");
	}

	GRAPHICS_CMD_LIST->SetGraphicsRootSignature(mRootSignature->GetRootSignature().Get());	// 루트 시그니처 set

	uint32 mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

	ID3D12DescriptorHeap* descHeap = mDescHeap->GetDescriptorHeap().Get();
	GRAPHICS_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	// 사용할 테이블 힙을 선택한다. 무거운 호출이므로 프레임당 1번 사용을 권장한다.

	// Table 바인딩
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 1, GBUFFER_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 2, GROUP_START, GROUP_COUNT);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 3, PARTICLE_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 4, PARTICLE_SPAWN_INDEX_START, 1);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 5, ANIMATION_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 6, TEXTURE_MATERIALS_INDEX_START);


}

void RenderManager::SetTable()
{
	uint32 mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

	if (mRootSignature == nullptr) {
		mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");
	}

	ID3D12DescriptorHeap* descHeap = mDescHeap->GetDescriptorHeap().Get();
	GRAPHICS_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	// 사용할 테이블 힙을 선택한다. 무거운 호출이므로 프레임당 1번 사용을 권장한다.

	// Table 바인딩
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 1, GBUFFER_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 2, GROUP_START, GROUP_COUNT);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 3, PARTICLE_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 4, PARTICLE_SPAWN_INDEX_START, 1);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 5, ANIMATION_INDEX_START);
	RENDERMANAGER.GetGraphicsDescHeap()->CommitGraphicsTable(mFrameCount, 6, TEXTURE_MATERIALS_INDEX_START);
}

void RenderManager::CreateRenderTargetGroups()
{
	// RenderScale 적용
	// 렌더 스케일은 씬 RT(Depth/G-buffer/Lighting/HDR)에만 적용. 
	// 스왑체인, UI, post는 mWindow 크기를 유지
	{
		// (scale이 1.0이면 render 크기와 window 크기가 같아서 기존과 동일)
		float s = mGraphicsSettings.renderScale;
		if (s < 0.25f) s = 0.25f;
		if (s > 1.0f)  s = 1.0f;
		mRenderWidth  = max(1u, static_cast<uint32>(mWindow.Width  * s + 0.5f));
		mRenderHeight = max(1u, static_cast<uint32>(mWindow.Height * s + 0.5f));
	}

	// DepthStencil
	shared_ptr<Texture> dsTexture = gEngine->GetResourceManager().CreateTexture(L"DepthStencil",
		DXGI_FORMAT_D32_FLOAT, mWindow.Width, mWindow.Height,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 0);

	shared_ptr<Texture> msaaDsTexture = nullptr;
	if (IsMsaaEnabled())
	{
		msaaDsTexture = gEngine->GetResourceManager().CreateTexture(L"DepthStencil_MSAA",
			DXGI_FORMAT_D32_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 0, mMsaaSampleCount, mMsaaQuality);
	}


	// SwapChain Group
	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainTarget_" + std::to_wstring(i);

			ComPtr<ID3D12Resource> resource;
			mSwapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));	// SwapChainBuffer를 가져온다.
			rtVec[i].Target = RESOURCEMANAGER.CreateTextureFromResource(name, resource, 0);	// SwapChainBuffer를 이용해서 Texture 생성
			// SwapChainBuffer를 이용해서 Texture 생성
		}


		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)].Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}

	// SwapChain MSAA Group
	if (IsMsaaEnabled())
	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainMsaaTarget_" + std::to_wstring(i);

			rtVec[i].Target = RESOURCEMANAGER.CreateTexture(name,
				DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
				CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, false, mMsaaSampleCount, mMsaaQuality);
		}

		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)].Create(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN, rtVec, msaaDsTexture);
	}


	//======공용=======
	
		// PRE_DEPTH
		shared_ptr<Texture> depthPreTexture = RESOURCEMANAGER.CreateTexture(
			L"SceneDepth",
			DXGI_FORMAT_R32_TYPELESS, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, true);

		{
			vector<RenderTarget> rtVec(0); // 컬러 RT 없음
			mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::PRE_DEPTH)]
				.Create(RENDER_TARGET_GROUP_TYPE::PRE_DEPTH, rtVec, depthPreTexture);
		}


	// Shadow Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT);

		shared_ptr<Texture> shadowDepthTexture = RESOURCEMANAGER.CreateTexture(L"ShadowDepthStencil",
			DXGI_FORMAT_R32_TYPELESS, 2048, 2048,   // CSM 해상도(RenderSystem.h shadowMapSize + utils.hlsl x2 동기)
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, true, 1, 0, Vec4(),
			RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT, TextureType::TEXTURE_2D_ARRAY);

		for (uint32 i = 0; i < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++i)
			rtVec[i].Target = shadowDepthTexture;

		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)].Create(RENDER_TARGET_GROUP_TYPE::SHADOW, rtVec, shadowDepthTexture);
	}

	// Deferred Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT);


		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"NormalTarget",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mRenderWidth, mRenderHeight,  // 뷰노멀과 metallic을 저장한다.
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[2].Target = RESOURCEMANAGER.CreateTexture(L"EmissiveTarget",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)].Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, depthPreTexture);
	}


	// Lighting Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"DiffuseLightTarget",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		rtVec[1].Target = RESOURCEMANAGER.CreateTexture(L"SpecularLightTarget",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,0);


		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)].Create(RENDER_TARGET_GROUP_TYPE::LIGHTING, rtVec, depthPreTexture);
	}
	// HDR Group
	{
		vector<RenderTarget> rtVec(1);

		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"HDRSceneTarget",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mRenderWidth, mRenderHeight,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);

		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::HDR)].Create(RENDER_TARGET_GROUP_TYPE::HDR, rtVec, depthPreTexture);
	}

	// PostProcess Group
	{
		vector<RenderTarget> rtVec(1);
		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PostHDRTargetA",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::POST_HDR_A)].Create(RENDER_TARGET_GROUP_TYPE::POST_HDR_A, rtVec, dsTexture);
	}

	{
		vector<RenderTarget> rtVec(1);
		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PostHDRTargetB",
			DXGI_FORMAT_R16G16B16A16_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::POST_HDR_B)].Create(RENDER_TARGET_GROUP_TYPE::POST_HDR_B, rtVec, dsTexture);
	}

	{
		vector<RenderTarget> rtVec(1);
		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PostLDRTargetA",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::POST_LDR_A)].Create(RENDER_TARGET_GROUP_TYPE::POST_LDR_A, rtVec, dsTexture);
	}

	{
		vector<RenderTarget> rtVec(1);
		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"PostLDRTargetB",
			DXGI_FORMAT_R8G8B8A8_UNORM, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::POST_LDR_B)].Create(RENDER_TARGET_GROUP_TYPE::POST_LDR_B, rtVec, dsTexture);
	}

	// Motion Vector Group
	{
		vector<RenderTarget> rtVec(1);
		rtVec[0].Target = RESOURCEMANAGER.CreateTexture(L"MotionVectorTarget",
			DXGI_FORMAT_R16G16_FLOAT, mWindow.Width, mWindow.Height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
		mRenderTargetGroup[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::MOTION_VECTOR)].Create(RENDER_TARGET_GROUP_TYPE::MOTION_VECTOR, rtVec, dsTexture);
	}

	// PRE_DEPTH SRV: rtVec가 비어 있으므로 루프 밖에서 별도 생성
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = mDescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
		uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC depthSrv = {};
		depthSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		depthSrv.Format = DXGI_FORMAT_R32_FLOAT;
		depthSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		depthSrv.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE srvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			cpuhandle, static_cast<uint32>(GBUFFER_INDEX::GBUFFER_PREDEPTH_INDEX) * srvSize);
		DEVICE->CreateShaderResourceView(depthPreTexture->GetTex2D().Get(), &depthSrv, srvhandle);

		// 슬롯(1)도 같은 depth SRV로 채움
		D3D12_CPU_DESCRIPTOR_HANDLE posAliasHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			cpuhandle, static_cast<uint32>(GBUFFER_INDEX::GBUFFER_POSITION_INDEX) * srvSize);
		DEVICE->CreateShaderResourceView(depthPreTexture->GetTex2D().Get(), &depthSrv, posAliasHandle);
	}

	int i = 0;
	for (auto& renderTargetGroup : mRenderTargetGroup) {

		if (renderTargetGroup.GetGroupType() == RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN
			|| renderTargetGroup.GetGroupType() == RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN
			|| renderTargetGroup.GetGroupType() == RENDER_TARGET_GROUP_TYPE::PRE_DEPTH) {
			continue;
		}


		for (auto& renderTarget : renderTargetGroup.GetRTG()) {


			if (renderTargetGroup.GetGroupType() == RENDER_TARGET_GROUP_TYPE::SHADOW)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC cascadeSrv = {};
				cascadeSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				cascadeSrv.Format = DXGI_FORMAT_R32_FLOAT;
				cascadeSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				cascadeSrv.Texture2DArray.MostDetailedMip = 0;
				cascadeSrv.Texture2DArray.MipLevels = 1;
				cascadeSrv.Texture2DArray.FirstArraySlice = 0;
				cascadeSrv.Texture2DArray.ArraySize = renderTarget.Target->GetTex2D()->GetDesc().DepthOrArraySize;
				cascadeSrv.Texture2DArray.PlaneSlice = 0;
				cascadeSrv.Texture2DArray.ResourceMinLODClamp = 0.0f;

				D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = mDescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
				uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				D3D12_CPU_DESCRIPTOR_HANDLE srvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuhandle, static_cast<uint32>(GBUFFER_INDEX::GBUFFER_CASCADE_INDEX) * srvSize);
				DEVICE->CreateShaderResourceView(renderTarget.Target->GetTex2D().Get(), &cascadeSrv, srvhandle);

				// SHADOW 그룹은 배열 하나를 공유하므로 SRV를 1회만 생성한다.
				// G_BUFFER 첫 멤버는 Normal(slot 2) — slot 1은 depth 알리아스로 이미 채움
				i = static_cast<uint32>(GBUFFER_INDEX::GBUFFER_NORMAL_INDEX);
				break;
			}

			UINT mipLevels = 1;	// 임시 밉맵

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
				srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스
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
			case D3D12_SRV_DIMENSION_TEXTURE2DMS:
				srvDesc.Texture2D.MostDetailedMip = 0;       // 가장 높은 해상도의 밉맵부터 시작
				srvDesc.Texture2D.MipLevels = mipLevels;     // 사용할 밉맵 레벨 수
				srvDesc.Texture2D.PlaneSlice = 0;            // 플레인 슬라이스
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 최소 LOD 클램프
				break;
			default:
				// 지원하지 않는 차원에 대한 오류 처리
				break;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = mDescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_CPU_DESCRIPTOR_HANDLE srvhandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuhandle, (i ) * srvSize);
			DEVICE->CreateShaderResourceView(renderTarget.Target->GetTex2D().Get(), &srvDesc, srvhandle);
			renderTarget.Target->SetSrvHandle(srvhandle);

			i++;
		}


	}

}
