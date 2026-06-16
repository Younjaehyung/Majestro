#pragma once
#include "Component.h"
#include <functional>

// 버튼의 현재 시각 상태
enum class ButtonVisualState : uint8
{
    Normal,
    Hovered,
    Pressed,
};

// UI 버튼 컴포넌트
// UITransformComponent + UICusSpriteComponent와 함께 사용
// UIButtonSystem이 매 프레임 마우스 히트테스트 + 시각 피드백 처리
class UIButtonComponent : public Component<UIButtonComponent>
{
public:
    // ── 입력 상태 ──────────────────────────────────────────────
    bool mHovered = false;   // 마우스가 버튼 위에 있는지
    bool mPressed = false;   // 현재 누르고 있는지 (Down 엣지 감지용)

    // ── 콜백 ───────────────────────────────────────────────────
    std::function<void()> mOnClick;       // 마우스 Down 엣지 시 발화
    std::function<void()> mOnHoverEnter;  // 호버 진입 시 1회
    std::function<void()> mOnHoverExit;   // 호버 이탈 시 1회

    // ── 효과음 키 (SfxTable.json) ──────────────────────────────
    // UIButtonSystem이 클릭/호버 시 SfxSystem::Play(key) 직접 호출
    std::string mClickSfxKey = "ui/select";   // 클릭 확정음
    std::string mHoverSfxKey = "ui/hover";    // 마우스 진입음

    // ── 상태별 Diffuse 색상 (UI_PS.hlsl에 전달) ────────────────
    Vec4 mNormalColor  = { 0.15f, 0.15f, 0.45f, 0.85f };
    Vec4 mHoveredColor = { 0.35f, 0.35f, 0.80f, 1.00f };
    Vec4 mPressedColor = { 0.08f, 0.08f, 0.25f, 1.00f };

    // ── 상태별 크기 배율 (mBaseSize 기준) ──────────────────────
    Vec2  mBaseSize      = { 0.f, 0.f }; // 생성 시 원본 크기 보관
    float mNormalScale   = 1.00f;
    float mHoveredScale  = 1.05f;
    float mPressedScale  = 0.97f;

    // ── VFX 버튼 전용 스케일 (UIVfxComponent.mScale에 적용) ───
    // UICusSpriteComponent 없이 VFX만 사용하는 버튼에 유용
    float mVfxNormalScale  = 1.00f;
    float mVfxHoveredScale = 1.15f;
    float mVfxPressedScale = 0.90f;

    // ── 내부 상태 (변경 시만 머티리얼/크기 갱신) ───────────────
    ButtonVisualState mPrevState = ButtonVisualState::Normal;

    // false 이면 UIButtonSystem 이 hit test/onClick 스킵
    bool mEnabled = true;
};
