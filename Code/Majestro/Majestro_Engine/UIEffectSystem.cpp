#include "pch.h"
#include "UIEffectSystem.h"

#include "World.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Timer.h"

#include "UIVfxComponent.h"
#include "Vfx.h"

UIEffectSystem::UIEffectSystem(World* world) : System::System(world)
{
	mPhase = SysPhase::Render;
	mOrder = 3;  // UIRenderSystem(2) 이후 → 스프라이트 위에 VFX
}

UIEffectSystem::~UIEffectSystem()
{
	if (mWorld && mWorld->HasComponentPool<UIVfxComponent>())
	{
		for (auto& e : mWorld->GetEntitiesWithComponent<UIVfxComponent>())
		{
			UIVfxComponent* comp = mWorld->GetComponent<UIVfxComponent>(e);
			if (comp && comp->efkHandle != -1)
				uiManager_->StopEffect(comp->efkHandle);
		}
	}
	uiManager_.Reset();
	setting_.Reset();
}

void UIEffectSystem::Initialize()
{
	auto renderer = RENDERMANAGER.GetEfkRenderer();

	setting_ = Effekseer::Setting::Create();
	setting_->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	uiManager_ = Effekseer::Manager::Create(2000);

	uiManager_->SetSpriteRenderer(renderer->CreateSpriteRenderer());
	uiManager_->SetRibbonRenderer(renderer->CreateRibbonRenderer());
	uiManager_->SetRingRenderer(renderer->CreateRingRenderer());
	uiManager_->SetTrackRenderer(renderer->CreateTrackRenderer());
	uiManager_->SetModelRenderer(renderer->CreateModelRenderer());

	uiManager_->SetTextureLoader(renderer->CreateTextureLoader());
	uiManager_->SetModelLoader(renderer->CreateModelLoader());
	uiManager_->SetMaterialLoader(renderer->CreateMaterialLoader());
	uiManager_->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

	LoadResources();
}

Effekseer::EffectRef UIEffectSystem::LoadEffect(const std::string_view path, float magnification)
{
	EfkString efkPath = ToEfkString(path);
	return Effekseer::Effect::Create(uiManager_, efkPath.c_str(), magnification);
}

Effekseer::Handle UIEffectSystem::Play(UIVfxComponent* comp)
{
	Effekseer::Handle handle = uiManager_->Play(
		comp->mVfx->mEffect,
		comp->mScreenX, comp->mScreenY, comp->mScreenZ);
	comp->mIsPlaying = true;
	comp->efkHandle  = handle;
	return handle;
}

// Render 페이즈 시스템은 Update(float)가 호출되지 않음 → 빈 구현
void UIEffectSystem::Update(float /*deltaTime*/) {}

void UIEffectSystem::Update()
{
	if (!mWorld->HasComponentPool<UIVfxComponent>()) return;

	float dt = DELTA_TIME;

	// --- 시뮬레이션 ---
	for (auto& e : mWorld->GetEntitiesWithComponent<UIVfxComponent>())
	{
		UIVfxComponent* comp = mWorld->GetComponent<UIVfxComponent>(e);
		if (comp == nullptr || comp->mVfx == nullptr) continue;

		// efkHandle == -1 이면 아직 미재생 → Play 호출
		// 씬에서 mIsPlaying=true로 미리 설정해도 정상 동작
		if (comp->efkHandle == -1)
			Play(comp);
	}

	// Manager Update와 SetTime은 프레임당 1회
	uiManager_->Update(dt * 60.f);
	mTotalTime += dt;
	RENDERMANAGER.GetEfkRenderer()->SetTime(mTotalTime);

	// --- 렌더링 ---
	auto renderer = RENDERMANAGER.GetEfkRenderer();

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
		uiManager_->Draw(drawParam);
		renderer->EndRendering();
	}
}

Effekseer::Matrix44 UIEffectSystem::BuildOrthoProjection()
{
	auto& window = RENDERMANAGER.GetWindow();
	float W = static_cast<float>(window.Width);
	float H = static_cast<float>(window.Height);

	// 픽셀 좌표 (0,0)~(W,H) → NDC (-1,1)
	// Effekseer LH 좌표계, Y축 스크린 방향(아래=+) 반전
	Effekseer::Matrix44 ortho{};
	ortho.Values[0][0] =  2.f / W;
	ortho.Values[1][1] = -2.f / H;
	ortho.Values[2][2] =  1.f;
	ortho.Values[3][0] = -1.f;
	ortho.Values[3][1] =  1.f;
	ortho.Values[3][3] =  1.f;
	return ortho;
}

void UIEffectSystem::LoadResources()
{
	// UIVfx 전용 리소스가 별도로 있다면 여기서 로드
}
