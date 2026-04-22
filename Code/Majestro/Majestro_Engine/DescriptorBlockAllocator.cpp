#include "pch.h"
#include "DescriptorBlockAllocator.h"

void DescriptorBlockAllocator::Initialize(uint32 startIndex, uint32 blockSize, uint32 blockCount)
{
	mBlockSize = blockSize;
	mFreeBlocks = {};
	mRetiredBlocks.clear();

	for (uint32 i = 0; i < blockCount; ++i)
		mFreeBlocks.push(startIndex + (i * blockSize));
}

DescriptorBlock DescriptorBlockAllocator::Allocate()
{
	if (mFreeBlocks.empty())
		return {};

	const uint32 startIndex = mFreeBlocks.front();
	mFreeBlocks.pop();

	return DescriptorBlock{ startIndex };
}

void DescriptorBlockAllocator::ReleaseDeferred(const DescriptorBlock& block, uint64 fenceValue)
{
	if (!block.IsValid())
		return;

	mRetiredBlocks.push_back({ block, fenceValue });
}

void DescriptorBlockAllocator::GarbageCollect(uint64 completedFenceValue)
{
	auto it = mRetiredBlocks.begin();
	while (it != mRetiredBlocks.end())
	{
		if (it->fenceValue <= completedFenceValue)
		{
			mFreeBlocks.push(it->block.startIndex);
			it = mRetiredBlocks.erase(it);
		}
		else
		{
			++it;
		}
	}
}
