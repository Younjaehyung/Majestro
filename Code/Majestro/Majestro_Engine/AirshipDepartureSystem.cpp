#include "pch.h"
#include "AirshipDepartureSystem.h"
#include "AirshipDepartureComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "CameraSystem.h"

std::vector<std::type_index> AirshipDepartureSystem::After() const
{
    return { typeid(CameraSystem) };
}

void AirshipDepartureSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<AirshipDepartureComponent>())
        return;

    const Entity singleton = mWorld->GetSingletonEntity();
    AirshipDepartureComponent* dep = mWorld->GetComponent<AirshipDepartureComponent>(singleton);
    if (!dep || !dep->mPlaying)
        return;

    Apply(dep, dt);

    // 마지막 키프레임 도달 시 재생 종료.
    // 이후 씬 전환은 재생을 시작시킨 NetRecvSystem 이 mPlaying 을 보고 이어받는다.
    if (dep->mElapsed >= dep->Duration())
        dep->mPlaying = false;
}

void AirshipDepartureSystem::Apply(AirshipDepartureComponent* dep, float dt)
{
    dep->mElapsed += dt;

    Cinematic::ApplyCameraSequence(mWorld, dep->mKeys, dep->mElapsed);
}
