#pragma once
#include "UIFeature.h"

class World;

class DamagePopupUpdateFeature : public UIFeature
{
public:
    void Update(float dt) override;

private:
    void ConsumeEvents();
    void UpdatePopups(float dt);

    bool ProjectWorldToScreen(const Vec3& worldPos,
        const Matrix& view,
        const Matrix& proj,
        float viewportW,
        float viewportH,
        Vec2& outScreenPos);
};
