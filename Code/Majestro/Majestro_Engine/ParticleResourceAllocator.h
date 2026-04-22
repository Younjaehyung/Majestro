#pragma once
#include <unordered_map>
#include "ParticleComponent.h"

class StructuredBuffer;

struct ParticleGpuAllocation
{
	shared_ptr<StructuredBuffer> particleBuffer;
	uint32 capacity = 0;

	bool IsValid() const
	{
		return particleBuffer != nullptr && capacity > 0;
	}
};

class ParticleResourceAllocator
{
public:
	void Initialize();
	ParticleGpuAllocation Allocate(uint32 requestedCount);
	void ReleaseDeferred(ParticleGpuAllocation&& allocation, uint64 fenceValue);
	void GarbageCollect(uint64 completedFenceValue);

private:
	array<uint32,7> mParticleBuckets = { 256, 512, 1024, 2048, 4096, 8192, 16384 };


	struct RetiredAllocation
	{
		ParticleGpuAllocation allocation;
		uint64 fenceValue = 0;
	};

	uint32 GetBucketCapacity(uint32 requestedCount) const;
	void ResetAllocation(ParticleGpuAllocation& allocation);

	std::unordered_map<uint32, std::vector<ParticleGpuAllocation>> mFreeLists;
	std::vector<RetiredAllocation> mRetiredAllocations;
};
