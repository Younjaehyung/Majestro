#pragma once
#include "Component.h"

constexpr int CIRC_VIS_POINTS = 128;  // 주파수 빈 수 (= 독립 quad 수)
constexpr int CIRC_MAX_SPIKES = 4;    // 동시 활성 스파이크 최대 수

// CircularVisualizerComponent
// 원형 오디오 파동 비주얼라이저 데이터 컴포넌트.
// CircularVisualizerSystem이 매 프레임 waveAmplitudes를 갱신(스파이크 효과 포함).
// CircularVisualizerPass가 이를 읽어 빈당 독립 quad(4버텍스)를 CPU에서 계산하고
// DrawIndexedInstanced 1회로 얇고 날카로운 파동 띠를 렌더링한다.
class CircularVisualizerComponent : public Component<CircularVisualizerComponent>
{
public:
    // 파동 진폭 배열 (0~1, 스파이크 효과 포함)
    // 각 인덱스 i는 각도 (2π * i / CIRC_VIS_POINTS) 에 대응
    std::array<float, CIRC_VIS_POINTS> waveAmplitudes{};

    // --- 레이아웃 (픽셀 단위) ---
    Vec2  center        = { 960.f, 540.f };  // 화면 중앙 픽셀 좌표
    float baseRadius    = 30.f;             // 기준 원 반지름 (px)
    float maxAmplitude  = 20.f;             // 진폭 픽셀 스케일 (amp * maxAmplitude = 돌출 px)

    // --- 빈 형태 ---
    // 0에 가까울수록 빈 사이 간격이 없어지고,
    // 0.5에 가까울수록 빈이 매우 얇은 선처럼 보인다.
    float binGapRatio   = 0.0f;  // 빈 사이 간격 비율 (0.0 ~ 0.5)

    // --- 비대칭 스무딩 ---
    float riseSmooth    = 40.f;   // 상승 속도 (1/s, 클수록 빠름)
    float fallSmooth    = 5.f;    // 하강 속도 (1/s)

    // --- 게인 ---
    float gain          = 12.f;   // FMOD 출력값 증폭 배율

    // --- 스파이크 ---
    float spikeThreshold    = 0.40f;  // 저주파 평균 에너지 임계값 (0~1)
    float spikeMultiplier   = 4.5f;   // 스파이크 진폭 배율 (일반 파동 대비 3~5배)
    float spikeDuration     = 0.20f;  // 스파이크 지속 시간 (초)
    float spikeCooldown     = 0.25f;  // 최소 스파이크 발생 간격 (초)

    // --- 내부 스파이크 상태 (System이 갱신, 외부 직접 수정 금지) ---
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
