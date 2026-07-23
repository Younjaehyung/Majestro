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


    auto cams = mWorld->GetEntitiesWithComponents<MainCameraComponent, CameraComponent, TransformComponent>();
    if (cams.empty())
        return;

    const Entity camE = cams.front();
    CameraComponent*    cam = mWorld->GetComponent<CameraComponent>(camE);
    TransformComponent* tr  = mWorld->GetComponent<TransformComponent>(camE);
    if (!cam || !tr)
        return;

    // 키프레임 구간 보간
    const Cinematic::CameraView view = Cinematic::SampleCameraSequence(dep->mKeys, dep->mElapsed);

    // Transform 적용
    tr->mLocalPosition = view.position;
    const Vec3 e = view.rotation.ToEuler();
    tr->mLocalRotationE = Vec3(XMConvertToDegrees(e.x),
                               XMConvertToDegrees(e.y),
                               XMConvertToDegrees(e.z));

    // FOV
    const float aspect  = cam->mWidth / cam->mHeight;
    const float hFovRad = XMConvertToRadians(view.fovDeg);
    const float vFovRad = 2.f * atanf(tanf(hFovRad * 0.5f) / aspect);
    cam->SetFOV(vFovRad);

    tr->FinalUpdate();
    cam->FinalUpdate(tr->GetWorldMatrix().Invert());
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
