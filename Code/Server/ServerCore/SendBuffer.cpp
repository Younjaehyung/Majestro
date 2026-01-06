#include "pch.h"
#include "SendBuffer.h"



void SendBufferManager::Initialize(size_t count) {

    for (size_t i = 0; i < count/2; ++i) {
        mPool.push(new SendBuffer());
    }
    mTotalAllocated = count;
    
}


void SendBufferManager::Shutdown() {

    while (!mPool.empty()) {
		SendBuffer* p = mPool.top();
        mPool.pop();
		delete p;
    }
}

SendBuffer* SendBufferManager::Acquire() {
    if (mPool.empty()) {
        // LOG_WARN("SendBufferPool Exhausted! Allocating new. All: [{}] \n", mTotalAllocated);
        mTotalAllocated++;
        return new SendBuffer();
    }
    // LIFO
    SendBuffer* p = mPool.top();
    mPool.pop();
    return p;
}
