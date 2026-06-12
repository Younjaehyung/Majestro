#pragma once
#include "Component.h"

constexpr int CIRC_VIS_POINTS = 128;  // 막대(바) 수 — 원 둘레에 균등 분배
constexpr int CIRC_MAX_SPIKES = 4;    // 동시 활성 스파이크 최대 수 (useSpikes 활성 시)

// CircularVisualizerComponent
// 원형 오디오 비주얼라이저 데이터 컴포넌트.
// UIAudioVisualizerFeature가 매 프레임 FFT 스펙트럼으로 waveAmplitudes를 갱신하고,
// CircularVisualizerPass가 이를 읽어 막대(방사형 사각형)를 CPU에서 계산해 렌더링한다.
//
// 막대 규칙:
//   - 각도: 2* pi * i / CIRC_VIS_POINTS (균등 분배, 랜덤 없음)
//   - 안쪽 끝은 항상 baseRadius에 고정 (피벗 = 원 내부쪽 끝점)
//   - 길이 = minBarLength + amp * (maxBarLength - minBarLength) — 바깥 방향으로만 성장
//   - 무음 시 minBarLength 길이의 짧은 점선 형태 링이 유지됨
class CircularVisualizerComponent : public Component<CircularVisualizerComponent>
{
public:
    // 막대별 진폭 (0~1, 스무딩 적용 후)
    // 각 인덱스 i는 각도 (2π * i / CIRC_VIS_POINTS) 에 대응
    std::array<float, CIRC_VIS_POINTS> waveAmplitudes{};

    // --- 레이아웃 (픽셀 단위) ---
    Vec2  center        = { 960.f, 540.f };  // 원 중심 픽셀 좌표
    float baseRadius    = 160.f;  // 막대 안쪽 끝이 고정되는 반지름 (px) — 중앙 원은 비워짐
    float minBarLength  = 4.f;    // 무음 시 최소 막대 길이 (px) — 기본 원형 형태 유지용
    float maxBarLength  = 90.f;   // 진폭 1일 때 최대 막대 길이 (px)
    float barWidth      = 3.f;    // 막대 두께 (px) — 둘레 간격보다 작아야 막대끼리 분리됨

    // --- 비대칭 스무딩 (attack 빠르게 / release 느리게) ---
    float riseSmooth    = 40.f;   // 상승 속도 (1/s, 클수록 빠름)
    float fallSmooth    = 8.f;    // 하강 속도 (1/s)

    // --- 게인 ---
    float gain          = 12.f;   // FMOD 출력값 증폭 배율

    // --- 스파이크 (선택 기능) ---
    // 미니멀 스타일 기본값은 비활성 — 켜면 저음 비트에 랜덤 막대가 순간 돌출
    bool  useSpikes         = false;
    float spikeThreshold    = 0.40f;  // 저주파 평균 에너지 임계값 (0~1)
    float spikeMultiplier   = 1.0f;   // 스파이크 진폭 배율 (1 = maxBarLength까지)
    float spikeDuration     = 0.20f;  // 스파이크 지속 시간 (초)
    float spikeCooldown     = 0.25f;  // 최소 스파이크 발생 간격 (초)

    // --- 내부 스파이크 상태 (Feature가 갱신, 외부 직접 수정 금지) ---
    struct SpikeInfo
    {
        int   pointIdx = -1;   // -1이면 비활성
        float strength = 0.f;  // 현재 강도 (1→0 감쇠)
        float timer    = 0.f;  // 남은 지속 시간 (초)
    };
    std::array<SpikeInfo, CIRC_MAX_SPIKES> spikes{};
    float cooldownTimer = 0.f;

    bool  isVisible     = true;
};
