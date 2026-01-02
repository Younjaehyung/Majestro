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


class SendBufferPool
{
public:
    explicit SendBufferPool(size_t bufferSize, size_t initialCount)
        : _bufferSize(bufferSize)
    {
        for (size_t i = 0; i < initialCount; ++i)
            _pool.push(Create());
    }

    ~SendBufferPool()
    {
        while (!_pool.empty())
        {
            SendBuffer* b = _pool.top();
            _pool.pop();
            delete[] b->data;
            delete b;
        }
    }

    SendBuffer* Acquire()
    {
        if (_pool.empty())
            return Create();
        SendBuffer* b = _pool.top();
        _pool.pop();
        b->size = 0;
        b->sent = 0;
        return b;
    }

    void Release(SendBuffer* b)
    {
        _pool.push(b);
    }

private:
    SendBuffer* Create()
    {
        SendBuffer* b = new SendBuffer();
        b->data = new char[_bufferSize];
        b->size = 0;
        b->sent = 0;
        return b;
    }

private:
    size_t _bufferSize;
    std::stack<SendBuffer*> _pool;
};
