#include "pch.h"
#include "AirshipDepartureSystem.h"
#include "AirshipDepartureComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "CameraSystem.h"
#include "EventManager.h"
#include "GameEvents.h"

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

    // 마지막 키프레임 도달 시 종료 + 예약된 씬 전환 요청
    if (dep->mElapsed >= dep->Duration())
        Finish(dep);
}

void AirshipDepartureSystem::Apply(AirshipDepartureComponent* dep, float dt)
{
    dep->mElapsed += dt;

    Cinematic::ApplyCameraSequence(mWorld, dep->mKeys, dep->mElapsed);
}

void AirshipDepartureSystem::Finish(AirshipDepartureComponent* dep)
{
    dep->mPlaying = false;

    mWorld->GetEventManager()->Enqueue(EvNetSceneChange{ dep->mTargetScene });
}
