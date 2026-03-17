#pragma once
#include "System.h"

// AudioVisualizerSystem
// Phase: Sim — 매 프레임 FMOD FFT DSP에서 스펙트럼 데이터를 읽어
// AudioVisualizerComponent의 barHeights를 갱신한다.
// 실제 렌더링은 UIRenderSystem 내부의 AudioVisualizerPass가 담당한다.
class AudioVisualizerSystem : public System
{
public:
    explicit AudioVisualizerSystem(World* world);
    virtual void Update(float dt) override;

private:
    // barIdx에 해당하는 FFT 빈 범위 반환 (로그 스케일 주파수 매핑)
    std::pair<int, int> GetBinRange(int barIdx, int totalBars, int spectrumSize, float sampleRate) const;
};
