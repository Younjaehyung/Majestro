#pragma once
#include "UIFeature.h"

class BossCutInFeature : public UIFeature
{
public:
    void Update(float dt) override;
    void SpriteRender(DirectX::SpriteBatch* spriteBatch) override;


    bool HidesHud() const override { return mActive; }

private:
    void ResolveBossType();   // 월드에서 보스 EnemyType 을 찾아 1회 캐시

    bool  mPrevDone = false;   // BossCinematic mDone 에지 감지용
    bool  mActive   = false;   // 컷인 재생 중
    float mElapsed  = 0.f;     // 컷인 시작 후 경과(초)
    int   mBossType = -1;      // 해석된 EnemyType(Brass/Dragon), -1=미해석

    // 타임라인(초)
    static constexpr float kInDur   = 0.34f;
    static constexpr float kHoldDur = 1.40f;
    static constexpr float kOutDur  = 0.42f;
    static constexpr float kTotal   = kInDur + kHoldDur + kOutDur;
};
