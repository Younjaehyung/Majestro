#pragma once
#include "UIFeature.h"

class UltimateCutInFeature : public UIFeature
{
public:
    void Update(float dt) override;
    void SpriteRender(DirectX::SpriteBatch* spriteBatch) override;

private:
    bool     mCasting    = false;
    uint8_t  mPlayerType = 0;
    float    mFadeGate   = 0.f; // 0~1 (in/out 보간)
    float    mElapsed    = 0.f; // 컷인이 보이는 동안 누적 (슬라이드/플래시 타이밍용)

    static constexpr float kFadeInSpeed  = 6.0f; // 1/s
    static constexpr float kFadeOutSpeed = 4.0f; // 1/s
};
