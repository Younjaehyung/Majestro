
///*----------------
//	SendBuffer
//-----------------*/
#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "Packet.h"

struct SendBuffer
{
	uint32  WritePos = 0;
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

class SendBufferManager
{
public:

    // Initialize the pool with a specific number of packets
    static void Initialize(size_t count);

    // Destroy all packets in the pool
    static void Shutdown();

    // packet acquire
    [[nodiscard("PacketBlock not return")]]
    static SendBuffer* Acquire();

    // packet release
    static void Release(SendBuffer* p) {
        if (!p) return;
        m_pool.push_back(p);
    }

    // Size of available packets in the pool
    static size_t GetAvailableCount() {
        return m_pool.size();
    }

private:
    // Vector를 스택처럼 사용 (Cache Friendly)
    static inline std::vector<SendBuffer*> m_pool;
    static inline size_t m_totalAllocated;
};
