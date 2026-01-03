#pragma once
#include <stack>
#include <queue>
#include "Packet.h"
class PacketPool
{
public:
	Packet* Acquire()
	{
		if (_pool.empty())
			return new Packet();
		Packet* p = _pool.top();
		_pool.pop();
		return p;
	}

	void Release(Packet* p)
	{
		_pool.push(p);
	}

private:
	std::stack<Packet*> _pool;
};


class ProcessPacket
{
public:
    ProcessPacket() {}
    ~ProcessPacket() {}
    void Process(BYTE* buffer, int32 len);
public:
    void ProcessSyncPacket(BYTE* buffer, int32 len) {};
    void ProcessInputPacket(BYTE* buffer, int32 len) {};
    void ProcessActionPacket(BYTE* buffer, int32 len) {};
private:
  //  PacketPool		    mPacketPool;			// 패킷 풀 recv패킷으로부터 할당(재사용)
    std::queue<Packet*> mCommandQueue;
};

