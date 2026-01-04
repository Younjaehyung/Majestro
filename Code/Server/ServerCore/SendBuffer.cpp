#include "pch.h"
#include "SendBuffer.h"



void SendBufferManager::Initialize(size_t count) {
    mPool.reserve(count);
    for (size_t i = 0; i < count/2; ++i) {
        mPool.push_back(new SendBuffer());
    }
    mTotalAllocated = count;
    
}


void SendBufferManager::Shutdown() {

    for (SendBuffer* p : mPool) {
        delete p;
    }
    mPool.clear();
    
}

SendBuffer* SendBufferManager::Acquire() {
    if (mPool.empty()) {
        LOG_WARN("SendBufferPool Exhausted! Allocating new. All: [{}] \n", mTotalAllocated);
        mTotalAllocated++;
        return new SendBuffer();
    }
    // LIFO
    SendBuffer* p = mPool.back();
    mPool.pop_back();
    return p;
}
