#include "pch.h"
#include "CloudDriftSystem.h"
#include "CloudDriftComponent.h"
#include "TransformComponent.h"

void CloudDriftSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<CloudDriftComponent>())
        return;

    for (Entity e : mWorld->View<CloudDriftComponent>())
    {
        CloudDriftComponent* cloud = mWorld->GetComponent<CloudDriftComponent>(e);
        TransformComponent*  tr    = mWorld->GetComponent<TransformComponent>(e);
        if (!cloud || !tr)
            continue;

        ApplyDrift(cloud, tr, dt);
    }
}

void CloudDriftSystem::ApplyDrift(CloudDriftComponent* cloud, TransformComponent* tr, float dt)
{
    cloud->mTravelled += kBaseSpeed * cloud->mSpeedScale * dt;
    if (cloud->mTravelled >= kRecycleDist)
        cloud->mTravelled -= kRecycleDist;

    const float alongOffset = cloud->mTravelled - kRecycleDist * 0.5f;

    tr->mLocalPosition = cloud->mBasePos + kTravelDir * alongOffset;
}
