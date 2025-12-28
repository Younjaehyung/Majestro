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
//
//좌상단 정렬 UI를 원함(피벗 0, 0)
//inst.Pivot = (0, 0)
//inst.Position = (100, 50)   [ UI의 좌상단이(100, 50)에 옴
//inst.Size = (256, 64)       [ == 256x64 px
//
//중앙 정렬 UI를 원함(피벗 0.5, 0.5)
//inst.Pivot = (0.5, 0.5)
//inst.Position = (960, 540)  [UI의 중앙이 화면 중앙에 옴(FullHD 기준)
//inst.Size = (256, 64)
//
//우하단 정렬(피벗 1, 1)
//inst.Pivot = (1, 1)
//inst.Position = (ScreenW - 20, ScreenH - 20) [ UI의 우하단 꼭짓점이 거기에 옴
//inst.Size = (256, 64)
//
