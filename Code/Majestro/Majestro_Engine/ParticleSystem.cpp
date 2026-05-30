#include "pch.h"
#include "Engine.h"
#include "ParticleSystem.h"

#include "GameEvents.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "ParticleEffect.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "TransformComponent.h"
#include "World.h"

ParticleSystem::ParticleSystem(World* world)
	: System(world)
{
	mPhase = SysPhase::Post;
}

void ParticleSystem::Initialize()
{
}

void ParticleSystem::Update(float deltaTime)
{
	ConsumeSpawnEvents();					// 파티클 이벤트 소모
	BuildActiveEmitters(deltaTime);			// 파티클 업데이트
	CollectDeadEmitters();					// 파티클 수명 종료된 이펙터 정리
}

const ParticleEmitterRuntime* ParticleSystem::GetRuntime(Entity entity) const
{
	auto it = mEmitterRuntime.find(entity.GetID());
	if (it == mEmitterRuntime.end())
		return nullptr;

	return &it->second;
}

Entity ParticleSystem::SpawnEffect(const wstring& effectName, const Vec3& worldPosition)
{
	Entity entity = mWorld->CreateEntity();

	TransformComponent transform{};
	transform.mLocalPosition = worldPosition;
	transform.mWorldPosition = worldPosition;
	transform.mWorldMatrix = Matrix::CreateTranslation(worldPosition);
	mWorld->AddComponent<TransformComponent>(entity, transform);

	ParticleComponent& particle = mWorld->AddComponent<ParticleComponent>(entity);
	particle.mEffectName = effectName;

	if (shared_ptr<ParticleEffect> effect = RESOURCEMANAGER.Get<ParticleEffect>(effectName))
	{
		ApplyEffectDesc(particle, effect->mDesc);
		particle.mEffectDescApplied = true;
	}

	return entity;
}

Entity ParticleSystem::SpawnAttachedEffect(const wstring& effectName, Entity followTarget, const Vec3& followOffset)
{
	Vec3 worldPosition = followOffset;
	if (TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(followTarget))
		worldPosition = targetTransform->mWorldPosition + followOffset;

	Entity entity = SpawnEffect(effectName, worldPosition);
	if (ParticleComponent* particle = mWorld->GetComponent<ParticleComponent>(entity))
	{
		particle->mFollowTarget = followTarget;
		particle->mFollowOffset = followOffset;
	}

	return entity;
}

void ParticleSystem::StopEffect(Entity entity)
{
	if (ParticleComponent* particle = mWorld->GetComponent<ParticleComponent>(entity))
	{
		particle->mIsPlaying = false;
		particle->mIsActive = false;
	}
}

void ParticleSystem::ConsumeSpawnEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (eventManager == nullptr)
		return;

	eventManager->Consume<EvSpawnParticleEffect>([this](const EvSpawnParticleEffect& event)
	{
		if (event.followTarget.IsValid())
			SpawnAttachedEffect(event.effectName, event.followTarget, event.followOffset);
		else
			SpawnEffect(event.effectName, event.worldPosition);
	});
}

void ParticleSystem::BuildActiveEmitters(float deltaTime)
{
	mActiveEmitters.clear();
	mAliveEntities.clear();

	if (!mWorld->HasComponentPool<ParticleComponent>() || !mWorld->HasComponentPool<TransformComponent>())
		return;

	uint32 emitterIndex = 0;
	for (Entity entity : mWorld->GetEntitiesWithComponents<ParticleComponent, TransformComponent>())
	{
		mAliveEntities.insert(entity.GetID());

		ParticleComponent* particle = mWorld->GetComponent<ParticleComponent>(entity);
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		if (particle == nullptr || transform == nullptr)
			continue;

		if (particle->mEffectDescApplied == false && particle->mEffectName.empty() == false)
		{
			if (shared_ptr<ParticleEffect> effect = RESOURCEMANAGER.Get<ParticleEffect>(particle->mEffectName))
			{
				ApplyEffectDesc(*particle, effect->mDesc);
				particle->mEffectDescApplied = true;
			}
		}

		if (!particle->mIsActive || !particle->mIsPlaying)
			continue;

		if (emitterIndex >= PARTICLE_EMITTER_COUNT)
			continue;

		UpdateFollowTarget(*particle, *transform);
		ParticleEmitterRuntime& runtime = EnsureEmitterRuntime(entity, *particle);
		if (!runtime.IsValid())
			continue;

		int addCount = 0;
		particle->mAccTime += deltaTime;
		if (particle->mLoop)
		{
			if (particle->mCreateInterval > 0.f)
			{
				while (particle->mAccTime >= particle->mCreateInterval)
				{
					particle->mAccTime -= particle->mCreateInterval;
					++addCount;
				}
			}
		}
		else if (particle->mBurstTriggered == false)
		{
			addCount = static_cast<int>(particle->mMaxParticle);
			particle->mBurstTriggered = true;
			particle->mAccTime = 0.f;
		}

		int textureIndex = -1;
		if (shared_ptr<Texture> texture = runtime.material->GetTexture(DIFFUSEMAP0INDEX))
			textureIndex = static_cast<int>(texture->GetImageIndex());

		ParticleSharedParams shared{};
		shared.MatWorld = transform->mWorldMatrix.Transpose();
		shared.TextureIndex = textureIndex;
		shared.maxCount = static_cast<int>(particle->mMaxParticle);
		shared.addCount = addCount;
		shared.deltaTime = deltaTime;
		shared.accTime = particle->mAccTime;
		shared.minLifeTime = particle->mMinLifeTime;
		shared.maxLifeTime = particle->mMaxLifeTime;
		shared.minSpeed = particle->mMinSpeed;
		shared.maxSpeed = particle->mMaxSpeed;
		shared.StartScale = particle->mStartScale;
		shared.EndScale = particle->mEndScale;

		mActiveEmitters.push_back({ entity, particle, shared, emitterIndex });
		++emitterIndex;
	}
}

void ParticleSystem::CollectDeadEmitters()
{
	mDeadEntities.clear();

	for (const auto& [entityId, runtime] : mEmitterRuntime)
	{
		if (mAliveEntities.contains(entityId) == false)
			mDeadEntities.push_back(entityId);
	}

	for (EntityID entityId : mDeadEntities)
	{
		auto it = mEmitterRuntime.find(entityId);
		if (it == mEmitterRuntime.end())
			continue;

		ReleaseEmitterRuntime(it->second);
		mEmitterRuntime.erase(it);
	}
}

ParticleEmitterRuntime& ParticleSystem::EnsureEmitterRuntime(Entity entity, ParticleComponent& component)
{
	ParticleEmitterRuntime& runtime = mEmitterRuntime[entity.GetID()];
	if (runtime.material == nullptr)
		runtime.material = RESOURCEMANAGER.Get<Material>(component.mMaterialName);
	if (runtime.computeShader == nullptr)
		runtime.computeShader = RESOURCEMANAGER.Get<Shader>(component.mComputeShaderName);
	if (runtime.mesh == nullptr || runtime.renderMode != component.mRenderMode)
	{
		runtime.renderMode = component.mRenderMode;
		runtime.mesh = RESOURCEMANAGER.Get<Mesh>(
			component.mRenderMode == ParticleComponent::RenderMode::InstancedQuadBillboard ? L"Rectangle" : L"Point");
	}

	if (!runtime.gpu.IsValid() || runtime.gpu.capacity < component.mMaxParticle)
	{
		if (runtime.gpu.IsValid())
			RENDERMANAGER.GetParticleResourceAllocator().ReleaseDeferred(
				std::move(runtime.gpu), RENDERMANAGER.GetLastGraphicsFenceValue());

		runtime.gpu = RENDERMANAGER.GetParticleResourceAllocator().Allocate(component.mMaxParticle);
	}

	if (!runtime.descriptorBlock.IsValid())
		runtime.descriptorBlock = RENDERMANAGER.GetParticleDescriptorAllocator().Allocate();

	if (runtime.descriptorBlock.IsValid() && runtime.gpu.IsValid())
	{
		runtime.gpu.particleBuffer->CreateSrvViewAtIndex(runtime.descriptorBlock.startIndex + static_cast<uint32>(PARTICLE_INDEX::SRV_PARTICLE_INDEX));
		runtime.gpu.particleBuffer->CreateUavViewAtIndex(runtime.descriptorBlock.startIndex + static_cast<uint32>(PARTICLE_INDEX::UAV_PARTICLE_INDEX));
	}

	return runtime;
}

void ParticleSystem::ReleaseEmitterRuntime(ParticleEmitterRuntime& runtime)
{
	RENDERMANAGER.GetParticleDescriptorAllocator().ReleaseDeferred(
		runtime.descriptorBlock,
		RENDERMANAGER.GetLastGraphicsFenceValue());

	if (runtime.gpu.IsValid())
	{
		RENDERMANAGER.GetParticleResourceAllocator().ReleaseDeferred(
			std::move(runtime.gpu),
			RENDERMANAGER.GetLastGraphicsFenceValue());
	}

	runtime.descriptorBlock = {};
}

void ParticleSystem::ApplyEffectDesc(ParticleComponent& component, const ParticleEffectDesc& desc)
{
	component.mMaterialName = desc.materialName;
	component.mComputeShaderName = desc.computeMaterialName;
	component.mMaxParticle = desc.maxParticle;
	component.mCreateInterval = desc.createInterval;
	component.mMinLifeTime = desc.minLifeTime;
	component.mMaxLifeTime = desc.maxLifeTime;
	component.mMinSpeed = desc.minSpeed;
	component.mMaxSpeed = desc.maxSpeed;
	component.mStartScale = desc.startScale;
	component.mEndScale = desc.endScale;
	component.mLoop = desc.loop;
	component.mAutoDestroy = desc.autoDestroy;
	component.mRenderMode = desc.renderMode;
}

void ParticleSystem::UpdateFollowTarget(ParticleComponent& component, TransformComponent& transform)
{
	if (!component.mFollowTarget.IsValid())
		return;

	TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(component.mFollowTarget);
	if (targetTransform == nullptr)
		return;

	const Vec3 worldPosition = targetTransform->mWorldPosition + component.mFollowOffset;
	transform.mLocalPosition = worldPosition;
	transform.mWorldPosition = worldPosition;
	transform.mWorldMatrix = targetTransform->mWorldMatrix;
	transform.mWorldMatrix._41 = worldPosition.x;
	transform.mWorldMatrix._42 = worldPosition.y;
	transform.mWorldMatrix._43 = worldPosition.z;
}
