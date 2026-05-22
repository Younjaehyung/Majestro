#pragma once
#include "World.h"
#include "UITransformComponent.h"

// 버튼 비주얼 종류 — 정적 텍스처 / Effekseer VFX / 스프라이트시트 애니메이션
enum class UIButtonVisual { Texture, Vfx, SpriteSheet };

struct UIButtonDesc
{
    // 레이아웃 (UITransformComponent)
    Anchor anchor   = Anchor::Center;
    Vec2   position {};
    Vec2   size     {};
    Vec2   pivot    = { 0.5f, 0.5f };
    uint8  layer    = 5;

    // 비주얼
    UIButtonVisual visual = UIButtonVisual::Texture;
    const wchar_t* resKey = nullptr;   // 리소스 이름

    Vec2  frameSize {};                // SpriteSheet 전용: 한 프레임 픽셀 크기
    int   frameCount = 1;              // SpriteSheet 전용: 프레임 수
    float animTime   = 1.f;            // SpriteSheet 전용: 한 바퀴 재생 시간(초)

    // 레이블 (nullptr 이면 텍스트 생략(순수 이미지 버튼))
    const wchar_t* label = nullptr;

    // 일반 크기 배율
    float normalScale  = 1.00f;
    float hoveredScale = 1.05f;
    float pressedScale = 0.97f;

    // 색상 틴트 피드백 (Texture/SpriteSheet 버튼)
    Vec4 normalColor  = Colors::White.v;
    Vec4 hoveredColor = Colors::White.v;
    Vec4 pressedColor = { 0.85f, 0.85f, 0.85f, 1.f };

    // VFX 이펙트 스케일 (Effekseer scale, 메인메뉴 기본값 보존)
    float vfxNormalScale  = 100.f;
    float vfxHoveredScale = 150.f;
    float vfxPressedScale = 130.f;


    std::function<void()> onClick;
};

Entity CreateUIButton(World* world, const UIButtonDesc& desc);
