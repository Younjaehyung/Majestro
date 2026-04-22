#pragma once

struct DescriptorBlock
{
	uint32 startIndex = std::numeric_limits<uint32>::max();

	bool IsValid() const
	{
		return startIndex != std::numeric_limits<uint32>::max();
	}
};


class DescriptorBlockAllocator
{
public:
	void Initialize(uint32 startIndex, uint32 blockSize, uint32 blockCount);
	DescriptorBlock Allocate();
	void ReleaseDeferred(const DescriptorBlock& block, uint64 fenceValue);
	void GarbageCollect(uint64 completedFenceValue);

private:
	struct RetiredBlock
	{
		DescriptorBlock block;
		uint64 fenceValue = 0;
	};

	uint32 mBlockSize = 0;
	std::queue<uint32> mFreeBlocks;
	std::vector<RetiredBlock> mRetiredBlocks;
};
