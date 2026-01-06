#pragma once
#include <vector>
#include <stack>
#include <mutex>

#include "Packet.h"

/*--------------
    RecvBuffer
----------------*/

class RecvBuffer
{
    enum { BUFFER_COUNT = 64 };

public:
    RecvBuffer(int32 bufferSize = 1024);
    ~RecvBuffer();

    void			Clean();
    bool			OnRead(int32 numOfBytes);
    bool			OnWrite(int32 numOfBytes);

    BYTE* ReadPos() { return &mBuffer[mReadPos]; }
    BYTE* WritePos() { return &mBuffer[mWritePos]; }
    int32			DataSize() { return mWritePos - mReadPos; }
    int32			FreeSize() { return mCapacity - mWritePos; }

private:
    int32			mCapacity = 0;
    int32			mBufferSize = 0;
    int32			mReadPos = 0;
    int32			mWritePos = 0;
    std::vector<BYTE>	mBuffer;
};



struct SendBuffer
{
    uint32  ReadPos = 0;
    uint32  Capacity = 0;
    uint8_t Data[MAX_PACKET_SIZE];          // 실제 데이터 버퍼

    void SetData(const void* data, uint16_t dataSize) {
        Capacity = dataSize;
        if (dataSize > 0 && data != nullptr) {
            std::memcpy(Data, data, dataSize);
        }
    }
};


class PacketPool
{
public:

	// Initialize the pool with a specific number of packets
    static void Initialize(size_t count) {
        std::lock_guard<std::mutex> lock(mMutex);
        m_pool.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            m_pool.push_back(new SendBuffer());
        }
        m_totalAllocated = count;
    }


	// Destroy all packets in the pool
    static void Shutdown() {
        std::lock_guard<std::mutex> lock(mMutex);
        for (SendBuffer* p : m_pool) {
            delete p;
        }
        m_pool.clear();
    }

	// packet acquire
    [[nodiscard("PacketBlock not return")]] 
    static SendBuffer* Acquire() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (m_pool.empty()) {

            LogDebug("[Warning] PacketPool Exhausted! Allocating new.\n");
            m_totalAllocated++;
            return new SendBuffer();
        }

        // LIFO
        SendBuffer* p = m_pool.back();
        m_pool.pop_back();
        return p;
    }

	// packet release
    static void Release(SendBuffer* p) {
        if (!p) return;

        std::lock_guard<std::mutex> lock(mMutex);
        m_pool.push_back(p);
    }

	// Size of available packets in the pool
    static size_t GetAvailableCount() {
        std::lock_guard<std::mutex> lock(mMutex);
        return m_pool.size();
    }

private:
    // Vector를 스택처럼 사용 (Cache Friendly)
    static inline std::vector<SendBuffer*> m_pool;
    static inline std::mutex mMutex;
    static inline size_t m_totalAllocated;
};



