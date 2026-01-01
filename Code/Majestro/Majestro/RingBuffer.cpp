#include "pch.h"
#include "RingBuffer.h"

bool RingBuffer::Push(const uint8_t* data, size_t size)
{
    
    if (GetFreeSize() < size) return false; // 공간 부족

    // 테일(Tail)부터 끝까지 남은 공간 계산
    size_t firstPart = min(size, m_capacity - m_tail);
    std::memcpy(&m_buffer[m_tail], data, firstPart);

    // 한 바퀴 돌아서(Wrap-around) 써야 하는 경우
    if (size > firstPart) {
        std::memcpy(&m_buffer[0], data + firstPart, size - firstPart);
    }

    m_tail = (m_tail + size) % m_capacity;
    return true;
    
}

void RingBuffer::Peek(uint8_t** outPtr1, size_t& outSize1, uint8_t** outPtr2, size_t& outSize2)
{
    size_t used = GetUsedSize();
    if (used == 0) {
        outSize1 = outSize2 = 0;
        return;
    }

    if (m_head < m_tail) {
        // 연속된 메모리
        *outPtr1 = &m_buffer[m_head];
        outSize1 = m_tail - m_head;
        outSize2 = 0;
    }
    else {
        // 끊어진 메모리 (Wrap-around 발생)
        *outPtr1 = &m_buffer[m_head];
        outSize1 = m_capacity - m_head;
        *outPtr2 = &m_buffer[0];
        outSize2 = m_tail;
    }
}
