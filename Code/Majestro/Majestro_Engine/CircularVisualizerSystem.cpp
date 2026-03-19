#include "pch.h"
#include "CircularVisualizerSystem.h"
#include "CircularVisualizerComponent.h"
#include "AudioManager.h"
#include "Engine.h"
#include "World.h"

#include <random>

// 저주파 에너지 감지에 사용할 포인트 수
static constexpr int kBassPoints = 6;

// 내부 처리 대역 수: AudioVisualizerSystem과 동일하게 32개 대역으로 처리해
// 각 대역이 충분히 넓어야 peak 값이 유의미하게 나온다.
// 이후 128개 포인트로 선형 보간 업샘플링하여 waveAmplitudes에 저장.
static constexpr int kInternalBands = 32;

CircularVisualizerSystem::CircularVisualizerSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
    mOrder = 100;  // AudioVisualizerSystem(99) 이후에 실행
}

void CircularVisualizerSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<CircularVisualizerComponent>())
        return;

    // FMOD FFT DSP에서 스펙트럼 데이터 폴링
    std::vector<float> spectrum;
    if (!AUDIOMANAGER.GetSpectrumData(spectrum))
        return;

    int specSize = static_cast<int>(spectrum.size());
    if (specSize == 0)
        return;

    float sampleRate = AUDIOMANAGER.GetSpectrumSampleRate();

    // 스파이크 위치 랜덤 생성용
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> countDist(2, 4);
    std::uniform_int_distribution<int> idxDist(0, CIRC_VIS_POINTS - 1);

    for (auto entity : mWorld->GetEntitiesWithComponent<CircularVisualizerComponent>())
    {
        auto* vis = mWorld->GetComponent<CircularVisualizerComponent>(entity);
        if (!vis || !vis->isVisible)
            continue;

        // ── 1. 스펙트럼 → 32개 내부 대역 처리 ────────────────────────────
        // 128개 포인트 직접 분할 시 각 대역이 너무 좁아 peak 값이 거의 0이 됨.
        // AudioVisualizerSystem과 동일하게 32개 대역을 사용해 충분한 에너지를 확보한 후
        // 128개 포인트로 선형 보간 업샘플링한다.
        float bands[kInternalBands] = {};
        for (int band = 0; band < kInternalBands; band++)
        {
            auto [b0, b1] = GetBinRange(band, kInternalBands, specSize, sampleRate);

            float peak = 0.f;
            for (int b = b0; b < b1; b++)
                peak = max(peak, spectrum[b]);

            // AudioVisualizerSystem과 동일한 EQ gain 적용
            float freqT  = static_cast<float>(band) / static_cast<float>(kInternalBands - 1);
            float eqGain = 0.5f + freqT * 3.5f;
            bands[band]  = std::clamp(peak * vis->gain * eqGain, 0.6f, 1.f);
        }

        // ── 2. 32개 → 128개 선형 보간 업샘플링 후 스무딩 적용 ───────────
        float bassEnergy = 0.f;
        for (int i = 0; i < CIRC_VIS_POINTS; i++)
        {
            // i를 kInternalBands 범위로 매핑
            float t   = static_cast<float>(i) / static_cast<float>(CIRC_VIS_POINTS - 1)
                        * static_cast<float>(kInternalBands - 1);
            int   lo  = static_cast<int>(t);
            int   hi  = min(lo + 1, kInternalBands - 1);
            float frac = t - static_cast<float>(lo);

            float target = bands[lo] * (1.f - frac) + bands[hi] * frac;
            float& cur   = vis->waveAmplitudes[i];

            // 비대칭 스무딩: 상승 빠르게(거친 느낌), 하강 천천히
            float speed = (target > cur) ? vis->riseSmooth : vis->fallSmooth;
            cur += (target - cur) * speed * dt;
            cur  = std::clamp(cur, 0.f, 1.f);

            if (i < kBassPoints)
                bassEnergy += cur;
        }
        bassEnergy /= static_cast<float>(kBassPoints);

        // ── 3. 스파이크 타이머 갱신 ────────────────────────────────────────
        vis->cooldownTimer -= dt;
        for (auto& sp : vis->spikes)
        {
            if (sp.pointIdx < 0)
                continue;

            sp.timer -= dt;
            if (sp.timer <= 0.f)
            {
                sp.pointIdx = -1;
                sp.strength = 0.f;
            }
            else
            {
                // 남은 시간 비율로 선형 감쇠
                sp.strength = sp.timer / vis->spikeDuration;
            }
        }

        // ── 4. 스파이크 발생 조건 판단 ────────────────────────────────────
        if (bassEnergy >= vis->spikeThreshold && vis->cooldownTimer <= 0.f)
        {
            vis->cooldownTimer = vis->spikeCooldown;

            int count = countDist(rng);  // 2~4개 랜덤
            int slot  = 0;
            for (auto& sp : vis->spikes)
            {
                if (slot >= count)
                    break;
                sp.pointIdx = idxDist(rng);
                sp.strength = 1.f;
                sp.timer    = vis->spikeDuration;
                slot++;
            }
        }

        // ── 5. 스파이크 진폭 적용 ─────────────────────────────────────────
        // waveAmplitudes > 1.0 을 허용하여 스파이크를 표현.
        // Pass에서 r = baseRadius + amp * maxAmplitude 로 계산되므로
        // amp = spikeMultiplier (예: 4.5) 이면 반지름이 일반 파동보다 4.5배 돌출된다.
        for (const auto& sp : vis->spikes)
        {
            if (sp.pointIdx < 0)
                continue;

            float& amp      = vis->waveAmplitudes[sp.pointIdx];
            float  spikeAmp = sp.strength * vis->spikeMultiplier;  // > 1 허용
            amp = max(amp, spikeAmp);
        }
    }
}

std::pair<int, int> CircularVisualizerSystem::GetBinRange(
    int pointIdx, int totalPoints, int spectrumSize, float sampleRate) const
{
    // 로그 스케일: 20Hz ~ 16kHz 를 CIRC_VIS_POINTS개 대역으로 분할
    constexpr float kMinHz = 20.f;
    constexpr float kMaxHz = 16000.f;
    const float logMin = std::log2f(kMinHz);
    const float logMax = std::log2f(kMaxHz);

    float t0 = static_cast<float>(pointIdx)     / static_cast<float>(totalPoints);
    float t1 = static_cast<float>(pointIdx + 1) / static_cast<float>(totalPoints);

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
