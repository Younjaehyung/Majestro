#include "pch.h"
#include "ParticleResourceAllocator.h"
#include "Buffer.h"


	

void ParticleResourceAllocator::Initialize()
{
	mFreeLists.clear();
	mRetiredAllocations.clear();
}

ParticleGpuAllocation ParticleResourceAllocator::Allocate(uint32 requestedCount)
{
	const uint32 bucketCapacity = GetBucketCapacity(requestedCount);
	auto& freeList = mFreeLists[bucketCapacity];

	if (!freeList.empty())
	{
		ParticleGpuAllocation allocation = std::move(freeList.back());
		freeList.pop_back();
		ResetAllocation(allocation);
		return allocation;
	}

	ParticleGpuAllocation allocation;
	allocation.capacity = bucketCapacity;
	allocation.particleBuffer = make_shared<StructuredBuffer>();
	allocation.particleBuffer->CreateDefaultBuffer(sizeof(ParticleStateParams), allocation.capacity);
	ResetAllocation(allocation);
	return allocation;
}

void ParticleResourceAllocator::ReleaseDeferred(ParticleGpuAllocation&& allocation, uint64 fenceValue)
{
	if (!allocation.IsValid())
		return;

	mRetiredAllocations.push_back({ std::move(allocation), fenceValue });
}

void ParticleResourceAllocator::GarbageCollect(uint64 completedFenceValue)
{
	auto it = mRetiredAllocations.begin();
	while (it != mRetiredAllocations.end())
	{
		if (it->fenceValue <= completedFenceValue)
		{
			mFreeLists[it->allocation.capacity].push_back(std::move(it->allocation));
			it = mRetiredAllocations.erase(it);
		}
		else
		{
			++it;
		}
	}
}

uint32 ParticleResourceAllocator::GetBucketCapacity(uint32 requestedCount) const
{
	for (uint32 bucket : mParticleBuckets)
	{
		if (requestedCount <= bucket)
			return bucket;
	}

	return requestedCount;
}

void ParticleResourceAllocator::ResetAllocation(ParticleGpuAllocation& allocation)
{
	std::vector<ParticleStateParams> zeroData(allocation.capacity);
	allocation.particleBuffer->PushDefaultToData(
		zeroData.data(),
		static_cast<uint32>(zeroData.size() * sizeof(ParticleStateParams)));
}
