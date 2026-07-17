#include "pch.h"
#include "MainMenuCameraSystem.h"
#include "MainMenuCameraComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "CameraSystem.h"
#include "MathUtils.h"

std::vector<std::type_index> MainMenuCameraSystem::Before() const
{
    return { typeid(CameraSystem) };
}

void MainMenuCameraSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<MainMenuCameraComponent>()) return;

    auto entities = mWorld->GetEntitiesWithComponents<
        MainMenuCameraComponent, TransformComponent, CameraComponent>();

    for (auto e : entities)
    {
        auto* mm  = mWorld->GetComponent<MainMenuCameraComponent>(e);
        auto* tr  = mWorld->GetComponent<TransformComponent>(e);
        auto* cam = mWorld->GetComponent<CameraComponent>(e);
        if (!mm || !tr || !cam) continue;
        if (!mm->mLoaded) continue;

        if (mm->mBlendT >= 1.f) continue; // 보간 끝 — 정지 유지

        // 진행
        const float step = (mm->mBlendDuration > 0.f) ? (dt / mm->mBlendDuration) : 1.f;
        mm->mBlendT = (std::min)(1.f, mm->mBlendT + step);

        const float s = MathUtils::SmoothStep01(mm->mBlendT);
        

        const auto& tgt = mm->View(mm->mTarget);
        Vec3       pos = Vec3::Lerp(mm->mFromPos, tgt.position, s);
        Quaternion rot = Quaternion::Slerp(mm->mFromRot, tgt.rotation, s);
        float      fov = mm->mFromFovDeg + (tgt.fovDeg - mm->mFromFovDeg) * s;

        tr->mLocalPosition = pos;
        Vec3 eR = rot.ToEuler();
        tr->mLocalRotationE = Vec3(XMConvertToDegrees(eR.x),
                                   XMConvertToDegrees(eR.y),
                                   XMConvertToDegrees(eR.z));

        // JSON fov 는 UE 가로 화각(도)에서 종횡비 기반 세로 화각(라디안)으로 변환
        const float aspect  = cam->mWidth / cam->mHeight;            // 16:9
        const float hFovRad = XMConvertToRadians(fov);
        const float vFovRad = 2.f * atanf(tanf(hFovRad * 0.5f) / aspect);
        cam->SetFOV(vFovRad);

        if (mm->mBlendT >= 1.f)
            mm->mCurrent = mm->mTarget;
    }
}
