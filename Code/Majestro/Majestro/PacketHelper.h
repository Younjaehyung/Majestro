#pragma once
#include "pch.h"
#include <stack>
#include <mutex>
#include "Packet.h"

struct PacketBlock
{
	PacketHeader Header;
	uint8_t Data[MAX_PACKET_SIZE];

    void SetData(PKT_Type packetId, const void* data, uint16_t dataSize) {
        Header.PacketType = packetId;
        Header.Size = sizeof(PacketHeader) + dataSize;
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
            m_pool.push_back(new PacketBlock());
        }
        m_totalAllocated = count;
    }


	// Destroy all packets in the pool
    static void Shutdown() {
        std::lock_guard<std::mutex> lock(mMutex);
        for (PacketBlock* p : m_pool) {
            delete p;
        }
        m_pool.clear();
    }

	// packet acquire
    [[nodiscard("PacketBlock not return")]] 
    static PacketBlock* Acquire() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (m_pool.empty()) {

            LogDebug("[Warning] PacketPool Exhausted! Allocating new.\n");
            m_totalAllocated++;
            return new PacketBlock();
        }

        // LIFO
        PacketBlock* p = m_pool.back();
        m_pool.pop_back();
        return p;
    }

	// packet release
    static void Release(PacketBlock* p) {
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
    static inline std::vector<PacketBlock*> m_pool;
    static inline std::mutex mMutex;
    static inline size_t m_totalAllocated;
};



