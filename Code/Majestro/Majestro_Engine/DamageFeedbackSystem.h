#pragma once
#include "System.h"

class World;
class TransformComponent;


//  hit flash, hit reaction. 향후 카메라 셰이크, 슬로우모, 사운드 트리거 등이 같이 묶일 자리.
class DamageFeedbackSystem : public System
{
public:
    DamageFeedbackSystem(World* world);

    void Update(float deltaTime) override;

private:

    // 단위 벡터 hitDirection을 타깃 entity 로컬 공간(yaw)으로 변환해 (pitch, yaw) 펄스 각도로 분해.
    // hitDirection이 0이면 단순 뒤로 끄덕임만 적용
    void DecomposeHitDirection(const Vec3& hitDirWorld,
        const TransformComponent* targetTr,
        float& outPitch, float& outYaw);

private:

    // 피격 펄스 피크 각도(라디안). 약 16~20도.
    constexpr static float mHitReactionPeakPitch = 0.35f; // 약 20도, 뒤로 젖힘 강도 기준
    constexpr static float mHitReactionPeakYaw = 0.28f; // 약 16도, 측면 비틂 강도 기준
};
