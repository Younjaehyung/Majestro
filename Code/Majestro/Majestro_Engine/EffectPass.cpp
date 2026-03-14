#include "pch.h"
#include "EffectPass.h"

#include "World.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"

#include "TransformComponent.h"
#include "VfxComponent.h"
#include "Vfx.h"

EffectPass::~EffectPass()
{
	if (mWorld && mWorld->HasComponentPool<VfxComponent>())
	{
		for (auto& e : mWorld->GetEntitiesWithComponent<VfxComponent>())
		{
			VfxComponent* comp = mWorld->GetComponent<VfxComponent>(e);
			if (comp && comp->efkHandle != -1)
				mManager->StopEffect(comp->efkHandle);
		}
	}
	mManager.Reset();
	mSetting.Reset();
}

void EffectPass::Initialize(World* world)
{
	mWorld = world;

	auto renderer = RENDERMANAGER.GetEfkRendererHDR();

	mSetting = Effekseer::Setting::Create();
	mSetting->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	mManager = Effekseer::Manager::Create(8000);

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

Effekseer::EffectRef EffectPass::LoadEffect(const std::string_view path, float magnification,
                                             const std::string_view materialPath)
{
	EfkString efkPath = ToEfkString(path);
	EfkString efkMat  = materialPath.empty() ? EfkString{} : ToEfkString(materialPath);
	const EFK_CHAR* matPtr = materialPath.empty() ? nullptr : efkMat.c_str();
	return Effekseer::Effect::Create(mManager, efkPath.c_str(), magnification, matPtr);
}

Effekseer::Handle EffectPass::Play(VfxComponent* comp, float x, float y, float z)
{
	Effekseer::Handle handle = mManager->Play(comp->mVfx->mEffect, x, y, z);
	comp->mIsPlaying = true;
	comp->efkHandle = handle;
	return handle;
}

Effekseer::Handle EffectPass::Play(VfxComponent* comp, const Effekseer::Vector3D& position)
{
	Effekseer::Handle handle = mManager->Play(comp->mVfx->mEffect, position);
	comp->mIsPlaying = true;
	comp->efkHandle = handle;
	return handle;
}

void EffectPass::Execute(float dt, const Effekseer::Matrix44& viewMat, const Effekseer::Matrix44& projMat)
{
	if (!mWorld->HasComponentPool<VfxComponent>()) return;

	// --- 시뮬레이션 ---
	for (Entity& e : mWorld->GetEntitiesWithComponent<VfxComponent>())
	{
		VfxComponent* comp = mWorld->GetComponent<VfxComponent>(e);
		if (comp == nullptr) continue;

		TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);

		if (!comp->mIsPlaying)
		{
			if (tr != nullptr)
				Play(comp, tr->mWorldPosition.x, tr->mWorldPosition.y, tr->mWorldPosition.z);
			else
				Play(comp, 0.f, 0.f, 0.f);
		}
		else if (comp->mIsLoop && !comp->mIsPaused && !mManager->Exists(comp->efkHandle))
		{
			// 재생이 끝났으면 처음부터 다시 재생
			comp->mIsPlaying = false;
			comp->mTotalTime = 0.f;
			if (tr != nullptr)
				Play(comp, tr->mWorldPosition.x, tr->mWorldPosition.y, tr->mWorldPosition.z);
			else
				Play(comp, 0.f, 0.f, 0.f);
		}

		mManager->SetPaused(comp->efkHandle, comp->mIsPaused);
		mManager->SetScale(comp->efkHandle, comp->mScale, comp->mScale, comp->mScale);

		if (!comp->mIsPaused)
		{
			if (tr != nullptr)
				comp->SetPosition(tr->mWorldPosition.x, tr->mWorldPosition.y, tr->mWorldPosition.z);
			comp->mTotalTime += dt;
		}
	}

	mManager->Update(dt * 60.f);

	auto renderer = RENDERMANAGER.GetEfkRendererHDR();
	renderer->SetTime(dt);

	// --- HDR RT에 렌더링 (ForwardPass 이후 SRV 상태 → RT로 전환) ---
	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargets();

	renderer->SetCameraMatrix(viewMat);
	renderer->SetProjectionMatrix(projMat);

	if (renderer->BeginRendering())
	{
		Effekseer::Manager::DrawParameter drawParam;
		drawParam.ZNear = 0.0f;
		drawParam.ZFar  = 1.0f;
		drawParam.ViewProjectionMatrix = renderer->GetCameraProjectionMatrix();
		mManager->Draw(drawParam);
		renderer->EndRendering();
	}

	// ToneMapPass가 HDR을 SRV로 읽으므로 RT→SRV 전환
	hdrGroup.WaitTargetToResource();
}

void EffectPass::LoadResources()
{
	auto& resources = RESOURCEMANAGER.GetAllResources<Vfx>();
	for (auto& res : resources)
	{
		shared_ptr<Vfx> effect = static_pointer_cast<Vfx>(res.second);
		if (effect)
			effect->mEffect = LoadEffect(ws2s(effect->mEffectPath));
	}
}
