#include "pch.h"
#include "AirshipDepartureSystem.h"
#include "AirshipDepartureComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "CameraSystem.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "SceneManager.h"

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

    // 마지막 키프레임 도달 시 종료 + 예약된 씬 전환 발화
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

    // 로딩 완료 후 스폰 요청
    if (dep->mNeedsGameStart)
    {
        if (auto* send = mWorld->GetSystemManager()->GetSystem<NetSendSystem>())
            send->RequestPendingGameStart();
    }

    gEngine->GetSceneManager().RequestSceneWithLoading(dep->mTargetScene, dep->mLoadingMessage);
}
