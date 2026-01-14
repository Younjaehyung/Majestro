#pragma once
#include <vector>
#include <atomic>
#include <mutex>

class RingBuffer {
public:
    RingBuffer(size_t capacity) : mCapacity(capacity), mHead(0), mTail(0) {
        mBuffer.resize(capacity);
    }

    // 데이터를 버퍼에 쓰기
    bool Push(const uint8_t* data, size_t size);

    // 데이터를 버퍼에서 읽기 (send용)
    // 링 버퍼의 특성상 메모리가 쪼개져 있을 수 있으므로 두 번에 나누어 보낼 수 있게 설계
    void Peek(uint8_t** outPtr1, size_t& outSize1, uint8_t** outPtr2, size_t& outSize2);

    void Consume(size_t size) {
        mHead = (mHead + size) % mCapacity;
    }

    size_t GetUsedSize() const {
        if (mTail >= mHead) return mTail - mHead;
        return mCapacity - (mHead - mTail);
    }

    size_t GetFreeSize() const {
        return mCapacity - GetUsedSize() - 1; // 1바이트는 Full/Empty 구분을 위해 비워둠
    }

private:
    std::vector<uint8_t> mBuffer;
    size_t mCapacity;
    size_t mHead; // 읽기 지점 (Consumer)
    size_t mTail; // 쓰기 지점 (Producer)
};
