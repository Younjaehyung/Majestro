#include "pch.h"
#include "EffectPass.h"

#include "World.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "MathUtils.h"
#include "Texture.h"

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
	mEffectCache.clear();
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
	// mManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

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

Effekseer::EffectRef EffectPass::LoadEffect(const shared_ptr<Vfx>& vfx)
{
	if (vfx == nullptr || vfx->mEffectPath.empty())
		return nullptr;

	auto it = mEffectCache.find(vfx->mEffectPath);
	if (it != mEffectCache.end())
		return it->second;

	Effekseer::EffectRef effect = LoadEffect(ws2s(vfx->mEffectPath));
	if (effect == nullptr)
	{
#ifdef _DEBUG
		OutputDebugStringW((L"EffectPass: failed to load Effekseer effect: " + vfx->mEffectPath + L"\n").c_str());
#endif
		return nullptr;
	}

	mEffectCache.emplace(vfx->mEffectPath, effect);
	return effect;
}

Effekseer::Handle EffectPass::Play(Entity owner, VfxComponent* comp, float x, float y, float z)
{
	Effekseer::EffectRef effect = LoadEffect(comp ? comp->mVfx : nullptr);
	if (effect == nullptr)
		return -1;

	Effekseer::Handle handle = mManager->Play(effect, x, y, z);

	mManager->SetUserData(handle, MakeOwnerToken(owner));
	comp->mIsPlaying = true;
	comp->efkHandle = handle;
	comp->mRootStopped = false;
	return handle;
}

Effekseer::Handle EffectPass::Play(Entity owner, VfxComponent* comp, const Effekseer::Vector3D& position)
{
	Effekseer::EffectRef effect = LoadEffect(comp ? comp->mVfx : nullptr);
	if (effect == nullptr)
		return -1;

	Effekseer::Handle handle = mManager->Play(effect, position);
	
	mManager->SetUserData(handle, MakeOwnerToken(owner));
	comp->mIsPlaying = true;
	comp->efkHandle = handle;
	comp->mRootStopped = false;
	return handle;
}

void EffectPass::Stop(Entity owner, VfxComponent* comp, bool allowRootStop)
{
	if (comp == nullptr)
		return;

	if (!IsInvalidHandle(comp->efkHandle) && mManager->Exists(comp->efkHandle) &&
		mManager->GetUserData(comp->efkHandle) == MakeOwnerToken(owner))
	{
		if (allowRootStop && comp->mStopRootWhenDisabled)
		{
			if (comp->mRootStopped == false)
			{
				mManager->StopRoot(comp->efkHandle);
				comp->mRootStopped = true;
			}
			return;
		}
		else
		{
		
			mManager->StopEffect(comp->efkHandle);
		}
	}
	else if (allowRootStop && comp->mStopRootWhenDisabled && comp->mRootStopped)
	{

		comp->efkHandle = -1;
		comp->mIsPlaying = false;
		comp->mTotalTime = 0.f;
		comp->mRootStopped = false;
		return;
	}

	comp->efkHandle = -1;
	comp->mIsPlaying = false;
	comp->mTotalTime = 0.f;
	comp->mRootStopped = false;
}

void EffectPass::Execute(float dt, const Effekseer::Matrix44& viewMat, const Effekseer::Matrix44& projMat, float zNear, float zFar)
{
	if (!mWorld->HasComponentPool<VfxComponent>()) return;

	// 시뮬레이션 
	for (Entity& e : mWorld->GetEntitiesWithComponent<VfxComponent>())
	{
		VfxComponent* comp = mWorld->GetComponent<VfxComponent>(e);
		if (comp == nullptr) continue;

		TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);

		
		Vec3 vfxWorldPos = Vec3::Zero;
		if (tr != nullptr)
		{
			vfxWorldPos = tr->mWorldPosition;

			// 부착 오프셋
			if (comp->mAttachOffset != Vec3::Zero)
			{
				// 대상 회전에 맞춰 월드 위치로 변환
				Vec3 scale; Quaternion rotQ; Vec3 trans;
				tr->mWorldMatrix.Decompose(scale, rotQ, trans); // 스케일 분리, 회전만 사용
				vfxWorldPos = tr->mWorldPosition + Vec3::Transform(comp->mAttachOffset, rotQ);
			}
		}

		if (!IsInvalidHandle(comp->efkHandle) && mManager->Exists(comp->efkHandle) &&
			mManager->GetUserData(comp->efkHandle) != MakeOwnerToken(e))
		{
			comp->efkHandle = -1;
			comp->mIsPlaying = false;
			comp->mTotalTime = 0.f;
			if (!comp->mRestartWhenFinished)
			{
				comp->mShouldPlay = false;
				if (comp->mAutoReturn)
					comp->mFinished = true;
				continue;
			}
		}

		if (comp->mVfx == nullptr || comp->mVfx->mEffectPath.empty())
		{
			Stop(e, comp, false);
			continue;
		}

		if (!comp->mShouldPlay)
		{
			Stop(e, comp);

			
			if (IsInvalidHandle(comp->efkHandle) || !mManager->Exists(comp->efkHandle) ||
				mManager->GetUserData(comp->efkHandle) != MakeOwnerToken(e))
			{
				continue;
			}
		}
		else if (!comp->mIsPlaying)
		{
			if (comp->efkHandle != -1)
				Stop(e, comp, false);

			Effekseer::Handle handle = -1;
			if (tr != nullptr)
				handle = Play(e, comp, vfxWorldPos.x, vfxWorldPos.y, vfxWorldPos.z);
			else
				handle = Play(e, comp, 0.f, 0.f, 0.f);
			if (handle == -1)
				continue;
		}
		else if (!comp->mIsPaused && comp->efkHandle != -1 && !mManager->Exists(comp->efkHandle))
		{
			comp->efkHandle = -1;
			comp->mIsPlaying = false;
			comp->mTotalTime = 0.f;
			comp->mRootStopped = false;

			if (comp->mRestartWhenFinished)
			{
				// 반복 재생이 필요한 고정 월드 VFX만 새 handle로 다시 시작
				Effekseer::Handle handle = -1;
				if (tr != nullptr)
					handle = Play(e, comp, vfxWorldPos.x, vfxWorldPos.y, vfxWorldPos.z);
				else
					handle = Play(e, comp, 0.f, 0.f, 0.f);
				if (handle == -1)
					continue;
			}
			else
			{
				// 자동 재시작하지 않는 VFX는 종료된 handle을 더 이상 갱신 X
				comp->mShouldPlay = false;
				if (comp->mAutoReturn)
					comp->mFinished = true;
				continue;
			}
		}

		if (IsInvalidHandle(comp->efkHandle) || !mManager->Exists(comp->efkHandle) ||
			mManager->GetUserData(comp->efkHandle) != MakeOwnerToken(e))
		{
			continue;
		}

		mManager->SetPaused(comp->efkHandle, comp->mIsPaused);
		if (comp->mUseWorldMatrix)
		{
			const Matrix effectMatrix = Matrix::CreateScale(comp->mScale) * comp->mWorldMatrix;
			mManager->SetMatrix(comp->efkHandle, ToEfkMatrix43(effectMatrix));
		}
		else
		{
			mManager->SetScale(comp->efkHandle, comp->mScale.x, comp->mScale.y, comp->mScale.z);
			if (tr != nullptr)
			{
				mManager->SetLocation(comp->efkHandle, vfxWorldPos.x, vfxWorldPos.y, vfxWorldPos.z);
				mManager->SetRotation(
					comp->efkHandle,
					tr->mLocalRotationE.x * kDegToRad,
					tr->mLocalRotationE.y * kDegToRad,
					tr->mLocalRotationE.z * kDegToRad);
			}
		}


		if (!comp->mIsPaused)
		{
			if (tr != nullptr)
				comp->SetPosition(vfxWorldPos.x, vfxWorldPos.y, vfxWorldPos.z);
			comp->mTotalTime += dt;
		}
	}

	mManager->Update(dt * 60.f);

	auto renderer = RENDERMANAGER.GetEfkRendererHDR();
	mTotalTime += dt;
	renderer->SetTime(mTotalTime);
	UpdateSceneTextures(renderer, projMat);

	// --- HDR RT에 렌더링 (ForwardPass 이후 SRV 상태 → RT로 전환) ---
	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargets();

	renderer->SetCameraMatrix(viewMat);
	renderer->SetProjectionMatrix(projMat);

	if (renderer->BeginRendering())
	{
		Effekseer::Manager::DrawParameter drawParam;
		
		drawParam.ZNear = max(0.0001f, zNear);
		drawParam.ZFar = max(drawParam.ZNear + 0.0001f, zFar);
		drawParam.ViewProjectionMatrix = renderer->GetCameraProjectionMatrix();
		mManager->Draw(drawParam);
		renderer->EndRendering();
	}

	// ToneMapPass가 HDR을 SRV로 읽으므로 RT→SRV 전환
	hdrGroup.WaitTargetToResource();
}

void EffectPass::UpdateSceneTextures(EffekseerRenderer::RendererRef renderer, const Effekseer::Matrix44& projMat)
{
	if (renderer == nullptr)
		return;

	auto& depthGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::PRE_DEPTH));
	Effekseer::Backend::TextureRef depthTexture = GetOrCreateEfkTexture(
		depthGroup.GetDSTexture(),
		mCachedDepthResource,
		mCachedDepthTexture);

	EffekseerRenderer::DepthReconstructionParameter depthParam{};
	depthParam.ProjectionMatrix33 = projMat.Values[2][2];
	depthParam.ProjectionMatrix34 = projMat.Values[2][3];
	depthParam.ProjectionMatrix43 = projMat.Values[3][2];
	depthParam.ProjectionMatrix44 = projMat.Values[3][3];
	renderer->SetDepth(depthTexture, depthParam);
	renderer->SetBackground(Effekseer::Backend::TextureRef{});
}

Effekseer::Backend::TextureRef EffectPass::GetOrCreateEfkTexture(shared_ptr<Texture> texture,
	ID3D12Resource*& cachedResource,
	Effekseer::Backend::TextureRef& cachedTexture)
{
	if (texture == nullptr || texture->GetTex2D() == nullptr)
	{
		cachedResource = nullptr;
		cachedTexture.Reset();
		return Effekseer::Backend::TextureRef{};
	}

	ID3D12Resource* resource = texture->GetTex2D().Get();
	if (resource != cachedResource || cachedTexture == nullptr)
	{
		cachedResource = resource;
#if defined(MAJESTRO_EFFEKSEER_EXTERNAL_TEXTURE)
		cachedTexture = EffekseerRendererDX12::CreateTexture(RENDERMANAGER.GetEfkGraphicsDevice(), resource);
#else
		cachedTexture.Reset();
#endif
	}

	return cachedTexture;
}

void EffectPass::LoadResources()
{
	mEffectCache.clear();

	auto& resources = RESOURCEMANAGER.GetAllResources<Vfx>();
	for (auto& res : resources)
	{
		shared_ptr<Vfx> effect = static_pointer_cast<Vfx>(res.second);
		if (effect)
			LoadEffect(effect);
	}
}
