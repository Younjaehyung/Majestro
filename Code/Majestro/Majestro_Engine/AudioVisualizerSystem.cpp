#include "pch.h"
#include "AudioVisualizerSystem.h"
#include "AudioVisualizerComponent.h"
#include "AudioManager.h"
#include "Engine.h"
#include "World.h"

AudioVisualizerSystem::AudioVisualizerSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
    mOrder = 99;  // 다른 Sim 시스템 이후에 실행
}

void AudioVisualizerSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<AudioVisualizerComponent>())
        return;

    // FMOD FFT DSP에서 스펙트럼 데이터 폴링
    std::vector<float> spectrum;
    if (!AUDIOMANAGER.GetSpectrumData(spectrum))
        return;

    int specSize = static_cast<int>(spectrum.size());
    if (specSize == 0)
        return;

    float sampleRate = AUDIOMANAGER.GetSpectrumSampleRate();

    for (auto entity : mWorld->GetEntitiesWithComponent<AudioVisualizerComponent>())
    {
        auto* vis = mWorld->GetComponent<AudioVisualizerComponent>(entity);
        if (!vis || !vis->isVisible)
            continue;

        for (int i = 0; i < VISUALIZER_BAR_COUNT; i++)
        {
            auto [binStart, binEnd] = GetBinRange(i, VISUALIZER_BAR_COUNT, specSize, sampleRate);

            // 해당 주파수 대역의 피크값 추출
            float peak = 0.f;
            for (int b = binStart; b < binEnd; b++)
                peak = max(peak, spectrum[b]);

            // 주파수 대역별 EQ gain: 저음(0.5×) ~ 고음(4.0×)
            // 저음역의 과도한 에너지를 억제하고 고음역을 부각시켜 균등한 시각적 분포를 만든다
            float freqT  = static_cast<float>(i) / static_cast<float>(VISUALIZER_BAR_COUNT - 1);
            float eqGain = 0.5f + freqT * 3.5f;
            float target = min(peak * vis->gain * eqGain, 1.f);
            float& cur   = vis->barHeights[i];

            // 비대칭 스무딩: 상승은 빠르게, 하강은 천천히
            float speed = (target > cur) ? vis->riseSmooth : vis->fallSmooth;
            cur += (target - cur) * speed * dt;
            cur  = std::clamp(cur, 0.f, 1.f);
        }
    }
}

std::pair<int, int> AudioVisualizerSystem::GetBinRange(
    int barIdx, int totalBars, int spectrumSize, float sampleRate) const
{
    // 로그 스케일: 40Hz ~ 18kHz 범위를 barCount개 대역으로 분할
    constexpr float kMinHz = 20.f;
    constexpr float kMaxHz = 16000.f;
    const float logMin = std::log2f(kMinHz);
    const float logMax = std::log2f(kMaxHz);

    float t0 = static_cast<float>(barIdx)     / static_cast<float>(totalBars);
    float t1 = static_cast<float>(barIdx + 1) / static_cast<float>(totalBars);

    float hz0 = std::pow(2.f, logMin + t0 * (logMax - logMin));
    float hz1 = std::pow(2.f, logMin + t1 * (logMax - logMin));

    // Hz → FFT 빈 인덱스 (FFT 빈 해상도 = sampleRate / (2 * spectrumSize))
    float hzPerBin = (sampleRate * 0.5f) / static_cast<float>(spectrumSize);
    int b0 = static_cast<int>(hz0 / hzPerBin);
    int b1 = static_cast<int>(hz1 / hzPerBin);

    b0 = std::clamp(b0, 0, spectrumSize - 1);
    b1 = std::clamp(b1 + 1, b0 + 1, spectrumSize);
    return { b0, b1 };
}
