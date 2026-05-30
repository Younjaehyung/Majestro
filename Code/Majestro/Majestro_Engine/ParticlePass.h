#pragma once
#include "RenderPass.h"
#include "ParticleComponent.h"

class ParticleSystem;
class Mesh;
struct ParticleEmitterRuntime;

struct ParticlePassRootConstants
{
	uint32 BaseInstanceID = 0;
	uint32 EmitterIndex = 0;
	uint32 Cascade = 0;
	uint32 PassCustomIndex = 0;
};

class ParticlePass
{
public:
	void Initialize(World* world);
	void Execute(const RenderContext& ctx);

private:
	void UploadSharedParams(uint32 frameIndex, float deltaTime);
	void RunEmitterCompute(const ParticleEmitterRuntime& runtime, uint32 emitterIndex, uint32 frameIndex);
	void DrawEmitter(const ParticleEmitterRuntime& runtime, uint32 emitterIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32 descriptorIndex) const;

private:
	uint32 mParticleRootGroup = 2;
	uint32 mParticleRootRuntime = 3;
	uint32 mParticleRootSpawn = 4;

	World* mWorld = nullptr;
	ParticleSystem* mParticleSystem = nullptr;
	std::vector<ParticleSharedParams> mSharedParams;
	std::vector<ParticleComputeSharedParams> mSpawnParams;
};
