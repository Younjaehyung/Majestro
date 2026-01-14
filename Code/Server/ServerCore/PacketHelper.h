#pragma once
#include <stack>
#include <queue>
#include <atomic>
#include <cstddef>
#include <type_traits>


//////////////////*
// Single Producer Single Consumer Ring Queue
// LOGIC THREAD <-> NETWORK THREAD
////////////////*/

struct SendBuffer;


struct InputCommand // Packet received (network thread -> logic thread)
{
    uint32 SessionId{};
    PKT_Type Type = PKT_Type::KNONE;
    MsgKind Kind = MsgKind::KNONE;
    uint32  SIze{};
    std::array<uint8, MAX_PACKET_SIZE> MsgBuffer{}; // 메시지 버퍼

    void Clear()
    {
        Type = PKT_Type::KNONE;
        Kind = MsgKind::KNONE;
        //SessionId = 0;
        SIze = 0;
    }

    template<typename T>
    bool StoreAs(const T& src)
    {
        // [추가] memcpy 저장은 T가 trivially copyable 이어야 안전
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        if (sizeof(T) > MAX_PACKET_SIZE) return false;
        std::memcpy(MsgBuffer.data(), &src, sizeof(T));
        SIze = static_cast<uint16_t>(sizeof(T));
        return true;
    }

    template<typename T>
    const T* ViewAs() const
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        if (SIze != sizeof(T)) return nullptr;
        return reinterpret_cast<const T*>(MsgBuffer.data());
    }
};

struct SendRequest { // Packet to be sent (logic thread -> network thread)

    uint32 SessionId{};
    PKT_Type Type = PKT_Type::KNONE;
    uint32  Size{};
    std::array<uint8, MAX_PACKET_SIZE> MsgBuffer{}; // 메시지 버퍼

    void Clear()
    {
        Type = PKT_Type::KNONE;
        //SessionId = 0;
        Size = 0;
    }

    template<typename T>
    bool StoreAs(const T& src)
    {
        // [추가] memcpy 저장은 T가 trivially copyable 이어야 안전
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        if (sizeof(T) > MAX_PACKET_SIZE) return false;
        std::memcpy(MsgBuffer.data(), &src, sizeof(T));
        Size = static_cast<uint16_t>(sizeof(T));
        return true;
    }

    template<typename T>
    const T* ViewAs() const
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        if (Size != sizeof(T)) return nullptr;
        return reinterpret_cast<const T*>(MsgBuffer.data());
    }
};

class SendRequestPacket
{
public:
    static bool SerializePacket(SendRequest& pkt, SendBuffer*);
    static void SerializeTcpPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeUdpPacket(SendRequest& pkt, SendBuffer*);

    
};


class ProcessPacket // Process received packets (network thread -> logic thread)
{
private:
public:
	static bool ProcessPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer,uint32 size);
	static void ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer, uint32 size);

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

