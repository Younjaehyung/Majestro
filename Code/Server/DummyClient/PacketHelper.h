#pragma once
#include <stack>
#include <queue>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include "../../Protocol/Packet.h"
//////////////////*
// Single Producer Single Consumer Ring Queue
// LOGIC THREAD <-> NETWORK THREAD
////////////////*/

struct SendBuffer;


struct InputCommand // Packet received (network thread -> logic thread)
{
    PKT_Type Type;
    uint32 SessionId;
    float moveX;
    float moveY;
    bool  action1;
    bool  action2;
};

struct SendRequest { // Packet to be sent (logic thread -> network thread)

    uint32 SessionId{};
    PKT_Type Type{};

    union
    {
		PacketTcpHeader tcpHeader;
		PacketUdpHeader udpHeader;
        
        C2S_InputPacket input;
        

        S2C_SyncPacket sync{};
    };

    SendRequest() :Type(PKT_Type::KNONE) {}
    SendRequest(PKT_Type t) : Type(t) {}
    SendRequest(PKT_Type t, const S2C_SyncPacket& s) : Type(t), sync(s) {}
    SendRequest(PKT_Type t, const PacketTcpHeader& th) : Type(t), tcpHeader(th) {}
    SendRequest(PKT_Type t, const PacketUdpHeader& uh) : Type(t), udpHeader(uh) {}
};

class SendRequestPacket
{
public:
    static bool SerializePacket(SendRequest& pkt, SendBuffer*);
    static void SerializeTcpPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeUdpPacket(SendRequest& pkt, SendBuffer*);


    static void SerializeSyncPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeInputPacket(SendRequest& pkt, SendBuffer*){}
    static void SerializeActionPacket(SendRequest& pkt, SendBuffer*){}
};


class ProcessPacket // Process received packets (network thread -> logic thread)
{
private:
public:
	static bool ProcessPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessLoginPacket(InputCommand& inputCommand, BYTE* buffer);
    static void ProcessSyncPacket(InputCommand& inputCommand, BYTE* buffer);
    static void ProcessInputPacket(InputCommand& inputCommand, BYTE* buffer) {}
    static void ProcessActionPacket(InputCommand& inputCommand, BYTE* buffer) {};
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
        mHead.store(0, std::memory_order_relaxed);
        mTail.store(0, std::memory_order_relaxed);
    }

    // Producer 전용
    bool Push(const T& item)
    {
        const size_t tail = mTail.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) & MASK;

        // 큐가 가득 참
        if (next == mHead.load(std::memory_order_acquire))
            return false;

        mBuffer[tail] = item;

        // item 쓰기 완료 후 tail 갱신
        mTail.store(next, std::memory_order_release);
        return true;
    }

    // Consumer 전용
    bool Pop(T& out)
    {
        const size_t head = mHead.load(std::memory_order_relaxed);

        // 큐가 비어 있음
        if (head == mTail.load(std::memory_order_acquire))
            return false;

        out = mBuffer[head];

        // 읽기 완료 후 head 갱신
        mHead.store((head + 1) & MASK, std::memory_order_release);
        return true;
    }

    // Consumer 전용 (읽기만, 제거 안 함)
    bool Peek(T& out) const
    {
        const size_t head = mHead.load(std::memory_order_relaxed);

        if (head == mTail.load(std::memory_order_acquire))
            return false;

        out = mBuffer[head];
        return true;
    }

    bool Empty() const
    {
        return mHead.load(std::memory_order_acquire) ==
            mTail.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t MASK = Capacity - 1;

    alignas(64) std::atomic<size_t> mHead;
    alignas(64) std::atomic<size_t> mTail;

    // false sharing 방지용 패딩은 alignas로 충분
    T mBuffer[Capacity];
};

