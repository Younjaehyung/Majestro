#pragma once

#include "UIFeature.h"

class UIHpBarComponent;
class UISpriteComponent;
class UITransformComponent;

class UIHpBarUpdateFeature : public UIFeature
{
public:
    void Update(float dt) override;
    void SpriteRender(DirectX::SpriteBatch* spriteBatch) override;
private:
    //Update
    void UpdateHpBarUI(float dt);
    void EnsureHpBarUIEntities(UIHpBarComponent* hpBar);
    void SetHpBarVisibility(UIHpBarComponent* hpBar, bool visible);
    void SpawnHpLossFragments(UIHpBarComponent* hpBar, const UISpriteComponent* fillSprite, const UITransformComponent* fillTransform, float oldRatio, float newRatio);
    void UpdateHpLossFragments(UIHpBarComponent* hpBar, float dt);

    // Render
    void DrawFragments(DirectX::SpriteBatch* spriteBatch, const class UIHpBarComponent* hpBar, const UISpriteComponent* fillSprite) const;
};
