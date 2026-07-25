#pragma once
#include "Component.h"

class BossTileGlowComponent : public Component<BossTileGlowComponent>
{
public:
    BossTileGlowComponent() = default;

    Vec3  mWarnColor{ 0.22f, 0.05f, 0.012f };   // 예고: 주황
    Vec3  mExplodeColor{ 0.60f, 0.25f, 0.09f }; // 폭발: 흰빛 섬광

    float mTargetIntensity{ 0.f };  // 목표 강도 (0이면 소등)
    float mCurrentIntensity{ 0.f }; // 페이드 결과
    float mRenderIntensity{ 0.f };  // 페이드 + 맥동 최종값

    float mFadeIn{ 0.12f };
    float mFadeOut{ 0.40f };

    // 예고 구간에서만 맥동. 폭발 직전일수록 빨라진다.
    bool  mPulsing{ false };
    float mPulseSpeed{ 5.0f };
    float mPulseDepth{ 0.35f };
    float mElapsed{ 0.f };

    Vec3  mColor{ 0.22f, 0.05f, 0.012f }; // 현재 적용 중인 색
};
