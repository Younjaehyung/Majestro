#pragma once
#include "System.h"

// CircularVisualizerSystem
// Phase: Sim — 매 프레임 FMOD FFT에서 스펙트럼 데이터를 읽어
// CircularVisualizerComponent의 waveAmplitudes를 갱신한다.
//
// 특징:
//   - 최소 스무딩(빠른 상승)으로 날카롭고 거친 원본 FFT 진폭 반영
//   - 저주파(베이스) 에너지 감지 시 2~4개 스파이크를 랜덤 위치에 발생
//   - 스파이크 진폭은 spikeDuration 동안 지수적으로 감쇠
class CircularVisualizerSystem : public System
{
public:
    explicit CircularVisualizerSystem(World* world);
    virtual void Update(float dt) override;

private:
    // pointIdx에 해당하는 FFT 빈 범위 반환 (로그 스케일 주파수 매핑)
    std::pair<int, int> GetBinRange(int pointIdx, int totalPoints,
                                    int spectrumSize, float sampleRate) const;
};
