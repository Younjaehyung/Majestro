#pragma once
#include "pch.h"
#include <stack>

struct PacketBlock
{
	uint16 capacity;   // 64 / 128 / 256 ...
	char   data[1];    // 가변 시작점
};

class PacketPool
{
private:

public:
	PacketBlock* Acquire();
	void Release(PacketBlock* block);

private:
	std::stack<PacketBlock*> freeList;
};


class PacketHelper
{ // EBR기반의 패킷 도우미 클래스
private:
	PacketPool pool64, pool128, pool256, pool512, pool1024;
public:

	PacketHelper() = default;
	~PacketHelper() = default;


	PacketBlock* Allocate(uint16 size);
	void Free(PacketBlock* block);

};

