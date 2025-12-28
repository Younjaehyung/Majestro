#pragma once
#include "Component.h"


enum class Anchor
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center
};

class UITransformComponent : public Component<UITransformComponent>
{
public:
    Anchor mAnchor = Anchor::TopLeft;

    uint8 mUILayerIndex = 0;

    Vec2 mPosition;     // anchor 기준 오프셋 (pixel)
    Vec2 mSize;         // pixel
    Vec2 mPivot;        // (0~1)
    Vec2 mFinalPixelPos;   // 최종 화면 픽셀 좌표
         
};

