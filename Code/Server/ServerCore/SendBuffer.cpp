#include "pch.h"
#include "SendBuffer.h"



void SendBufferManager::Initialize(size_t count) {
    m_pool.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        m_pool.push_back(new SendBuffer());
    }
    m_totalAllocated = count;
    
}


void SendBufferManager::Shutdown() {

    for (SendBuffer* p : m_pool) {
        delete p;
    }
    m_pool.clear();
    
}

SendBuffer* SendBufferManager::Acquire() {
    if (m_pool.empty()) {
        LOG_WARN("SendBufferPool Exhausted! Allocating new.\n");
        m_totalAllocated++;
        return new SendBuffer();
    }
    // LIFO
    SendBuffer* p = m_pool.back();
    m_pool.pop_back();
    return p;
}
