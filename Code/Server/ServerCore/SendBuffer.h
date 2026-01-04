#pragma once
///*----------------
//	SendBuffer
// Send시 Packet 관리용 버퍼
//-----------------*/
#include <vector>
#include <memory>
#include <algorithm>
#include "PacketHelper.h"
#include "Packet.h"

struct SendBuffer   // SEND BUFFER
{
	uint32  ReadPos = 0;                    
    uint32  Capacity = 0;
	uint8   Data[MAX_PACKET_SIZE];          // 실제 데이터 버퍼

    void SetData(const void* data, uint32 dataSize) {
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
        mPool.push_back(p);
    }

    // Size of available packets in the pool
    static size_t GetAvailableCount() {
        return mPool.size();
    }

private:
    // Vector를 스택처럼 사용 (Cache Friendly)
    static inline std::vector<SendBuffer*> mPool;
    static inline size_t mTotalAllocated;
};
