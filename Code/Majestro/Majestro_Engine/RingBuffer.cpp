#include "pch.h"
#include "RingBuffer.h"

bool RingBuffer::Push(const uint8_t* data, size_t size)
{
    
    if (GetFreeSize() < size) return false; // 공간 부족

    // 테일(Tail)부터 끝까지 남은 공간 계산
    size_t firstPart = min(size, mCapacity - mTail);
    std::memcpy(&mBuffer[mTail], data, firstPart);

    // 한 바퀴 돌아서(Wrap-around) 써야 하는 경우
    if (size > firstPart) {
        std::memcpy(&mBuffer[0], data + firstPart, size - firstPart);
    }

    mTail = (mTail + size) % mCapacity;
    return true;
    
}

void RingBuffer::Peek(uint8_t** outPtr1, size_t& outSize1, uint8_t** outPtr2, size_t& outSize2)
{
    size_t used = GetUsedSize();
    if (used == 0) {
        outSize1 = outSize2 = 0;
        return;
    }

    if (mHead < mTail) {
        // 연속된 메모리
        *outPtr1 = &mBuffer[mHead];
        outSize1 = mTail - mHead;
        outSize2 = 0;
    }
    else {
        // 끊어진 메모리 (Wrap-around 발생)
        *outPtr1 = &mBuffer[mHead];
        outSize1 = mCapacity - mHead;
        *outPtr2 = &mBuffer[0];
        outSize2 = mTail;
    }
}
