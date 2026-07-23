#include "pch.h"
#include "CloudDriftSystem.h"
#include "CloudDriftComponent.h"
#include "TransformComponent.h"
#include "TagComponent.h"   // MainCameraComponent

void CloudDriftSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<CloudDriftComponent>())
        return;

    // 활성 카메라 위치(빌보드가 향할 대상). 없으면 원점 기준으로 대체.
    Vec3 camPos = Vec3::Zero;
    auto cams = mWorld->GetEntitiesWithComponents<MainCameraComponent, TransformComponent>();
    if (!cams.empty())
    {
        if (TransformComponent* camTr = mWorld->GetComponent<TransformComponent>(cams.front()))
            camPos = camTr->GetWorldPosition();
    }

    for (Entity e : mWorld->View<CloudDriftComponent>())
    {
        CloudDriftComponent* cloud = mWorld->GetComponent<CloudDriftComponent>(e);
        TransformComponent*  tr    = mWorld->GetComponent<TransformComponent>(e);
        if (!cloud || !tr)
            continue;

        ApplyDrift(cloud, tr, camPos, dt);
    }
}

void CloudDriftSystem::ApplyDrift(CloudDriftComponent* cloud, TransformComponent* tr, const Vec3& camPos, float dt)
{
    // 전후 축으로만 누적 이동. 순환 거리를 넘으면 레인 시작으로 되돌린다(자립 순환).
    cloud->mTravelled += kBaseSpeed * cloud->mSpeedScale * dt;
    if (cloud->mTravelled >= kRecycleDist)
        cloud->mTravelled -= kRecycleDist;

    // 레인 중앙 기준 ±(순환거리/2) → 되돌아오는 지점이 양 끝(화면 밖)에 오게 한다.
    const float alongOffset = cloud->mTravelled - kRecycleDist * 0.5f;

    // 좌우(X)/높이(Y)는 mBasePos 고정, 전후(Z)로만 이동.
    const Vec3 pos = cloud->mBasePos + kTravelDir * alongOffset;

    // yaw 빌보드: 카드 로컬 +Z 가 카메라를 향하게 세운다(수평 회전만 → 항상 똑바로 선다).
    const Vec3  d = camPos - pos;
    const float yawDeg = XMConvertToDegrees(atan2f(d.x, d.z)) + kFrontFlipDeg;

    tr->mLocalPosition  = pos;
    tr->mLocalRotationE = Vec3(0.f, yawDeg, 0.f);   // TransformSystem(Post)이 월드 행렬을 다시 만든다.
}
