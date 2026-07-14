#pragma once
#include "Component.h"
#include "Entity.h"


class DecalComponent : public Component<DecalComponent>
{
public:
    Vec3  Center          = Vec3(0.0f, 0.0f, 0.0f); // 월드 중심 (OneShotPulse는 스폰 시 고정)
    Vec3  Normal          = Vec3(0.0f, 1.0f, 0.0f); // 투영축(표면 노멀).
    float Radius          = 5.0f;                    // 면(투영축 수직 평면) half-size
    float Height          = 3.0f;                    // 투영축 방향 half-extent(슬래브 두께)
    float Thickness       = 0.3f;                    // 링 두께 (월드 단위)
    Vec4  Color           = Vec4(1.0f, 1.0f, 1.0f, 1.0f); // HDR 색/틴트 (>1이면 Bloom)
    float NormalThreshold = 0.4f;                    // 투영축 정렬 하한 (abs(dot(surfaceN, Normal)))
    int   TexIndex        = -1;   

    // 수명 / 페이드 (초)
    float FadeIn   = 0.15f;   // 등장 페이드 인
    float FadeOut  = 0.3f;    // 소멸 페이드 아웃 (Lifetime 기준)
    float Lifetime = -1.0f;   // -1 = 무한
    float Elapsed  = 0.0f;    // 경과 시간

	// follow target
    Entity FollowTarget{};                       // 무효(기본)면 고정
    Vec3   FollowOffset = Vec3(0.0f, 0.0f, 0.0f); // 대상 기준 오프셋
    bool   DestroyWithTarget = false;            // 대상 소멸 시 이 데칼도 자동 파괴(소유 로직 없을 때)

    bool  Visible  = true;
};
