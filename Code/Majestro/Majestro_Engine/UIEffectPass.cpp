#include "pch.h"
#include "UIEffectPass.h"

#include "World.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"

#include "UIVfxComponent.h"
#include "Vfx.h"

UIEffectPass::~UIEffectPass()
{
	if (mWorld && mWorld->HasComponentPool<UIVfxComponent>())
	{
		for (auto& e : mWorld->GetEntitiesWithComponent<UIVfxComponent>())
		{
			UIVfxComponent* comp = mWorld->GetComponent<UIVfxComponent>(e);
			if (comp && comp->efkHandle != -1)
				mManager->StopEffect(comp->efkHandle);
		}
	}
	mManager.Reset();
	mSetting.Reset();
}

void UIEffectPass::Initialize(World* world)
{
	mWorld = world;

	auto renderer = RENDERMANAGER.GetEfkRendererUI();

	mSetting = Effekseer::Setting::Create();
	mSetting->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	mManager = Effekseer::Manager::Create(2000);

	mManager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
	mManager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
	mManager->SetRingRenderer(renderer->CreateRingRenderer());
	mManager->SetTrackRenderer(renderer->CreateTrackRenderer());
	mManager->SetModelRenderer(renderer->CreateModelRenderer());

	mManager->SetTextureLoader(renderer->CreateTextureLoader());
	mManager->SetModelLoader(renderer->CreateModelLoader());
	mManager->SetMaterialLoader(renderer->CreateMaterialLoader());
	mManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

	LoadResources();
}

Effekseer::EffectRef UIEffectPass::LoadEffect(const std::string_view path, float magnification)
{
	EfkString efkPath = ToEfkString(path);
	return Effekseer::Effect::Create(mManager, efkPath.c_str(), magnification);
}

Effekseer::Handle UIEffectPass::Play(UIVfxComponent* comp)
{
	Effekseer::Handle handle = mManager->Play(
		comp->mVfx->mEffect,
		comp->mScreenX, comp->mScreenY, comp->mScreenZ);
	comp->mIsPlaying = true;
	comp->efkHandle  = handle;
	return handle;
}

void UIEffectPass::Execute(float dt)
{
	if (!mWorld->HasComponentPool<UIVfxComponent>()) return;

	// --- 시뮬레이션 ---
	for (auto& e : mWorld->GetEntitiesWithComponent<UIVfxComponent>())
	{
		UIVfxComponent* comp = mWorld->GetComponent<UIVfxComponent>(e);
		if (comp == nullptr || comp->mVfx == nullptr) continue;

		if (!comp->mIsPlaying)
			Play(comp);
		else if (comp->mIsLoop && !comp->mIsPaused && !mManager->Exists(comp->efkHandle))
		{
			comp->mIsPlaying = false;
			comp->mTotalTime = 0.f;
			Play(comp);
		}

		mManager->SetPaused(comp->efkHandle, comp->mIsPaused);
		mManager->SetScale(comp->efkHandle, comp->mScale, comp->mScale, comp->mScale);

		if (!comp->mIsPaused)
			comp->mTotalTime += dt;
	}

	mManager->Update(dt * 60.f);
	mTotalTime += dt;

	auto renderer = RENDERMANAGER.GetEfkRendererUI();
	renderer->SetTime(mTotalTime);

	// --- SwapChain RT에 렌더링 (ToneMap 이후, UI 위에 표시) ---
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);

	Effekseer::Matrix44 identity{};
	identity.Values[0][0] = 1.f;
	identity.Values[1][1] = 1.f;
	identity.Values[2][2] = 1.f;
	identity.Values[3][3] = 1.f;

	renderer->SetCameraMatrix(identity);
	renderer->SetProjectionMatrix(BuildOrthoProjection());

	if (renderer->BeginRendering())
	{
		Effekseer::Manager::DrawParameter drawParam;
		drawParam.ZNear = 0.0f;
		drawParam.ZFar  = 1.0f;
		drawParam.ViewProjectionMatrix = renderer->GetCameraProjectionMatrix();
		mManager->Draw(drawParam);
		renderer->EndRendering();
	}
	// SwapChain은 RT 상태 유지 — 별도 barrier 불필요
}

Effekseer::Matrix44 UIEffectPass::BuildOrthoProjection()
{
	auto& window = RENDERMANAGER.GetWindow();
	float W = static_cast<float>(window.Width);
	float H = static_cast<float>(window.Height);

	// Y 반전 없음 → Effekseer +Y = 화면 위
	// 위치 지정 시 mScreenY = (H - pixelY) 로 변환해서 사용
	Effekseer::Matrix44 ortho{};
	ortho.Values[0][0] =  2.f / W;
	ortho.Values[1][1] =  2.f / H;
	ortho.Values[2][2] =  1.f;
	ortho.Values[3][0] = -1.f;
	ortho.Values[3][1] = -1.f;
	ortho.Values[3][3] =  1.f;
	return ortho;
}

void UIEffectPass::LoadResources()
{
	// UIVfx 전용 리소스가 별도로 있다면 여기서 로드
}
