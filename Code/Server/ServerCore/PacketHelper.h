#pragma once
#include <stack>
#include <queue>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include "Packet.h"

//////////////////*
// Single Producer Single Consumer Ring Queue
// LOGIC THREAD <-> NETWORK THREAD
////////////////*/

struct InputCommand // Packet received (network thread -> logic thread)
{
    int   sessionId;
    float moveX;
    float moveY;
    bool  action1;
    bool  action2;
};

class SendRequest { // Packet to be sent (logic thread -> network thread)
public:
    uint32 SessionId;
    uint32 Len;
    BYTE Data[128];
private:

};


template<typename T, size_t Capacity>
class SpscRingQueue // LOGIC <-> NETWORK
{
    static_assert(Capacity >= 2, "Capacity must be >= 2");
    static_assert((Capacity& (Capacity - 1)) == 0,
        "Capacity must be power of two");

public:
    SpscRingQueue()
    {
        _head.store(0, std::memory_order_relaxed);
        _tail.store(0, std::memory_order_relaxed);
    }

    // Producer 전용
    bool Push(const T& item)
    {
        const size_t tail = _tail.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) & MASK;

        // 큐가 가득 참
        if (next == _head.load(std::memory_order_acquire))
            return false;

        _buffer[tail] = item;

        // item 쓰기 완료 후 tail 갱신
        _tail.store(next, std::memory_order_release);
        return true;
    }

    // Consumer 전용
    bool Pop(T& out)
    {
        const size_t head = _head.load(std::memory_order_relaxed);

        // 큐가 비어 있음
        if (head == _tail.load(std::memory_order_acquire))
            return false;

        out = _buffer[head];

        // 읽기 완료 후 head 갱신
        _head.store((head + 1) & MASK, std::memory_order_release);
        return true;
    }

    // Consumer 전용 (읽기만, 제거 안 함)
    bool Peek(T& out) const
    {
        const size_t head = _head.load(std::memory_order_relaxed);

        if (head == _tail.load(std::memory_order_acquire))
            return false;

        out = _buffer[head];
        return true;
    }

    bool Empty() const
    {
        return _head.load(std::memory_order_acquire) ==
            _tail.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t MASK = Capacity - 1;

    alignas(64) std::atomic<size_t> _head;
    alignas(64) std::atomic<size_t> _tail;

    // false sharing 방지용 패딩은 alignas로 충분
    T _buffer[Capacity];
};

