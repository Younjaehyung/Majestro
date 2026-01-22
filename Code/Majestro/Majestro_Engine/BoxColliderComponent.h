#pragma once
#include "Component.h"

class BoxColliderComponent : public Component<BoxColliderComponent>
{
public:
    BoxColliderComponent() {}
    BoxColliderComponent(Vec3 half): HalfExtents(half){}
    BoxColliderComponent(Vec3 half, Vec3 center): HalfExtents(half),  Center(center){}

public:
    // [설명] 로컬 공간 기준 박스(OBB의 로컬 정의)
    // Center: 로컬 중심 오프셋
    // HalfExtents: 로컬 반길이 (x,y,z)
    Vec3 Center = Vec3(0, 0, 0);
    Vec3 HalfExtents = Vec3(10.5f, 10.5f, 10.5f);

    // [옵션] 디버그 표시용
    bool bDebugDraw = true;
    bool bNoDepth = false; // true면 항상 보이게(Depth Test X)
};