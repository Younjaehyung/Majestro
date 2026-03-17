#pragma once
#include "Component.h"
#include "IMGUIComponent.h"

constexpr int VISUALIZER_BAR_COUNT = 32;

// 오디오 비주얼라이저 데이터 컴포넌트
// AudioVisualizerSystem이 매 프레임 barHeights를 갱신하고,
// AudioVisualizerPass가 이 값을 읽어 UIInfo 버퍼에 직렬화한다.
class AudioVisualizerComponent : public Component<AudioVisualizerComponent>
{
public:
    // 주파수 대역별 현재 바 높이 (0~1 정규화, 스무딩 적용 후)
    std::array<float, VISUALIZER_BAR_COUNT> barHeights{};

    // 레이아웃 (픽셀 단위)
    Vec2  basePosition  = { 960.f, 1060.f };  // 바 하단 중앙 기준 픽셀 좌표
    float barWidth      = 6.f;
    float barSpacing    = 1.f;
    float maxBarHeight  = 250.f;

    // 비대칭 스무딩 계수
    float riseSmooth    = 25.f;   // 상승 속도 (빠르게)
    float fallSmooth    = 5.f;    // 하강 속도 (천천히)

    // 스펙트럼 게인 (FMOD 출력값은 매우 작으므로 증폭 필요)
    float gain          = 8.f;

    bool  isVisible     = true;
};

#ifdef _IMGUI
// AudioVisualizerComponent를 Inspector에서 실시간으로 편집하기 위한 IMGUI 컴포넌트.
// IMGUIComponent 대신 Component<AudioVisualizerIMGUI>를 직접 상속해 ECS static_assert를 통과한다.
// IMGUISystem에서 이 타입을 별도로 순회한다.
class AudioVisualizerIMGUI : public Component<AudioVisualizerIMGUI>
{
public:
    AudioVisualizerComponent* target = nullptr;

    void Draw()
    {
        if (!target) return;
        std::vector<EditorProperty> props;
        props.push_back({ "Base Position",  PropertyType::Vec2,  &target->basePosition,  0.f,    0.f });
        props.push_back({ "Bar Width",      PropertyType::Float, &target->barWidth,       1.f,   50.f });
        props.push_back({ "Bar Spacing",    PropertyType::Float, &target->barSpacing,     0.f,   20.f });
        props.push_back({ "Max Height",     PropertyType::Float, &target->maxBarHeight,   10.f, 800.f });
        props.push_back({ "Gain",           PropertyType::Float, &target->gain,           0.1f,  30.f });
        props.push_back({ "Rise Smooth",    PropertyType::Float, &target->riseSmooth,     1.f,   50.f });
        props.push_back({ "Fall Smooth",    PropertyType::Float, &target->fallSmooth,     0.1f,  20.f });
        props.push_back({ "Visible",        PropertyType::Bool,  &target->isVisible,      0.f,    0.f });
        mProps = std::move(props);
    }

    std::vector<EditorProperty> mProps;
};
#endif
