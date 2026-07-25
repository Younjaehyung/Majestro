#pragma once
#include "UIFeature.h"
#include "BossCutInComponent.h"

class BossCutInFeature : public UIFeature
{
public:
    void Update(float dt) override;
    void SpriteRender(DirectX::SpriteBatch* spriteBatch) override;
    bool RendersInGroup(UIRenderGroup group) const override { return group == UIRenderGroup::Cinematic; }

private:
    void ResolveBossType(BossCutInComponent* cutIn);

    bool mPrevDone = false;

    // 타임라인(초)
    static constexpr float kInDur   = 0.34f;
    static constexpr float kHoldDur = 1.40f;
    static constexpr float kOutDur  = 0.42f;
    static constexpr float kTotal   = kInDur + kHoldDur + kOutDur;
};
