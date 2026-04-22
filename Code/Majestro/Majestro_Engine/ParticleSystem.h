#pragma once
#include <unordered_map>
#include <unordered_set>
#include "System.h"
#include "ParticleComponent.h"
#include "ParticleEffect.h"
#include "DescriptorBlockAllocator.h"
#include "ParticleResourceAllocator.h"

class Material;
class Mesh;

struct ParticleEmitterRuntime
{
	DescriptorBlock descriptorBlock;
	ParticleGpuAllocation gpu;
	shared_ptr<Material> material;
	shared_ptr<Material> computeMaterial;
	shared_ptr<Mesh> mesh;

	bool IsValid() const
	{
		return descriptorBlock.IsValid() && gpu.IsValid() && material != nullptr && computeMaterial != nullptr && mesh != nullptr;
	}
};

struct ActiveParticleEmitter
{
	Entity entity;
	ParticleComponent* component = nullptr;
	ParticleSharedParams sharedParams;
	uint32 emitterIndex = 0;
};

class ParticleSystem : public System
{
public:
	ParticleSystem(World* world);

	void Initialize() override;
	void Update(float deltaTime) override;

	const std::vector<ActiveParticleEmitter>& GetActiveEmitters() const { return mActiveEmitters; }
	bool HasActiveEmitters() const { return mActiveEmitters.empty() == false; }
	const ParticleEmitterRuntime* GetRuntime(Entity entity) const;

	Entity SpawnEffect(const wstring& effectName, const Vec3& worldPosition);
	Entity SpawnAttachedEffect(const wstring& effectName, Entity followTarget, const Vec3& followOffset = Vec3::Zero);
	void StopEffect(Entity entity);

private:
	void ConsumeSpawnEvents();
	void BuildActiveEmitters(float deltaTime);
	void CollectDeadEmitters();
	ParticleEmitterRuntime& EnsureEmitterRuntime(Entity entity, ParticleComponent& component);
	void ReleaseEmitterRuntime(ParticleEmitterRuntime& runtime);
	void ApplyEffectDesc(ParticleComponent& component, const ParticleEffectDesc& desc);
	void UpdateFollowTarget(ParticleComponent& component, class TransformComponent& transform);

private:
	std::unordered_map<EntityID, ParticleEmitterRuntime> mEmitterRuntime;
	std::vector<ActiveParticleEmitter> mActiveEmitters;
	std::unordered_set<EntityID> mAliveEntities;
	std::vector<EntityID> mDeadEntities;
};
