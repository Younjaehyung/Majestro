#pragma once
#include <vector>
#include <atomic>
#include <mutex>

class RingBuffer {
public:
    RingBuffer(size_t capacity) : m_capacity(capacity), m_head(0), m_tail(0) {
        m_buffer.resize(capacity);
    }

    // 데이터를 버퍼에 쓰기
    bool Push(const uint8_t* data, size_t size);

    // 데이터를 버퍼에서 읽기 (send용)
    // 링 버퍼의 특성상 메모리가 쪼개져 있을 수 있으므로 두 번에 나누어 보낼 수 있게 설계
    void Peek(uint8_t** outPtr1, size_t& outSize1, uint8_t** outPtr2, size_t& outSize2);

    void Consume(size_t size) {
        m_head = (m_head + size) % m_capacity;
    }

    size_t GetUsedSize() const {
        if (m_tail >= m_head) return m_tail - m_head;
        return m_capacity - (m_head - m_tail);
    }

    size_t GetFreeSize() const {
        return m_capacity - GetUsedSize() - 1; // 1바이트는 Full/Empty 구분을 위해 비워둠
    }

private:
    std::vector<uint8_t> m_buffer;
    size_t m_capacity;
    size_t m_head; // 읽기 지점 (Consumer)
    size_t m_tail; // 쓰기 지점 (Producer)
};
