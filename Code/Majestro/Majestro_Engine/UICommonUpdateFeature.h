#pragma once

#include "UIFeature.h"

class UICommonUpdateFeature : public UIFeature
{
public:
    void Update(float dt) override;

private:
    void UpdateScripts(float dt);
    void UpdateSpriteAnimation(float dt);
    void UpdateTextContext(float dt);
};
