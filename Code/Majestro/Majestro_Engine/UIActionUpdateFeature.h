#pragma once

#include "UIFeature.h"

class UIActionUpdateFeature : public UIFeature
{
public:
    void Update(float dt) override;

private:
    void UpdateActiveUIEntities(float dt);
};
