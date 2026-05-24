#pragma once
#include "Component.h"
#include "Entity.h"


class DamagePopupComponent : public Component<DamagePopupComponent>
{
public:
    DamagePopupComponent() = default;

    // 따라갈 대상 엔티티 (피격된 적).
    Entity mAnchor;

    // 앵커의 월드 위치에 더해질 오프셋
    Vec3 mWorldOffset{ 0.f, 150.f, 0.f };

    // 매 초마다 더해지는 월드 속도 — 살짝 위로 떠오르는 연출
    Vec3 mFloatVelocity{ 0.f, 60.f, 0.f };

    float mAge{ 0.f };
    float mLifetime{ 0.8f };

    int32 mDamageValue{ 0 };

    // 엔티티가 파괴된 이후에도 마지막 알려진 월드 위치를 유지하기 위한 캐시
    Vec3 mLastWorldPos{ Vec3::Zero };
    bool mHasLastWorldPos{ false };
};
