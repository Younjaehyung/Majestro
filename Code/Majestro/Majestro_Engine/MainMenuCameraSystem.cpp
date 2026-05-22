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

        const float s = SmoothStep01(mm->mBlendT);
        

        const auto& tgt = mm->View(mm->mTarget);
        Vec3       pos = Vec3::Lerp(mm->mFromPos, tgt.position, s);
        Quaternion rot = Quaternion::Slerp(mm->mFromRot, tgt.rotation, s);
        float      fov = mm->mFromFovDeg + (tgt.fovDeg - mm->mFromFovDeg) * s;

        tr->mLocalPosition = pos;
        Vec3 eR = rot.ToEuler();
        tr->mLocalRotationE = Vec3(XMConvertToDegrees(eR.x),
                                   XMConvertToDegrees(eR.y),
                                   XMConvertToDegrees(eR.z));

        cam->SetFOV(XMConvertToRadians(fov * 0.5f));

        if (mm->mBlendT >= 1.f)
            mm->mCurrent = mm->mTarget;
    }
}
