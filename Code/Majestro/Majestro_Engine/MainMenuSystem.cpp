#include "pch.h"
#include "MainMenuSystem.h"
#include "Engine.h"
#include "InputManager.h"
#include "MainMenuController.h"
#include "MainMenuCameraComponent.h"
#include "MainMenuCameraSystem.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "UIButtonSystem.h"
#include "UIButtonComponent.h"
#include "UISpriteComponent.h"
#include "UITransformComponent.h"
#include "UIVfxComponent.h"
#include "UITextComponent.h"
#include "MathUtils.h"


std::vector<std::type_index> UIMainMenuSystem::After() const
{
    return { typeid(UIButtonSystem) };
}

std::vector<std::type_index> UIMainMenuSystem::Before() const
{
    return { typeid(MainMenuCameraSystem) };
}

bool UIMainMenuSystem::DetectAnyInputDown() const
{
    if (INPUT.GetMouseLeftDown() || INPUT.GetMouseRightDown() || INPUT.GetMouseMiddleDown() || INPUT.GetAnyKeyDown())
        return true;
   
    return false;
}

void UIMainMenuSystem::SetEntitiesVisible(const std::vector<Entity>& es, bool visible)
{
    for (Entity e : es)
    {
        if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = visible;
        if (auto* vf = mWorld->GetComponent<UIVfxComponent>(e))    vf->mVisible = visible;
        if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = visible;
        if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e)) bt->mEnabled = visible;
    }
}

void UIMainMenuSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<MainMenuController>()) return;

    auto entities = mWorld->GetEntitiesWithComponents<
        MainMenuController, MainMenuCameraComponent,
        TransformComponent, CameraComponent>();

    for (auto e : entities)
    {
        auto* ctrl = mWorld->GetComponent<MainMenuController>(e);
        auto* cam  = mWorld->GetComponent<MainMenuCameraComponent>(e);
        auto* tr   = mWorld->GetComponent<TransformComponent>(e);
        auto* cc   = mWorld->GetComponent<CameraComponent>(e);
        if (!ctrl || !cam || !tr || !cc) continue;

        // Title  |입력 감지 시 MainMenu 로
        if (ctrl->mState == MainMenuState::Title && !ctrl->mHasPending)
        {
            if (DetectAnyInputDown())
                ctrl->Request(MainMenuState::MainMenu);
        }

        // 배경 페이드+줌
        if (ctrl->mActiveBg.IsValid() && cam->mBlendT >= 1.f)
        {
            auto* bgSp = mWorld->GetComponent<UISpriteComponent>(ctrl->mActiveBg);
            auto* bgTr = mWorld->GetComponent<UITransformComponent>(ctrl->mActiveBg);
            if (bgSp && bgTr)
            {
                bgSp->mVisible = true;
                ctrl->mBgFadeT = (std::min)(1.f, ctrl->mBgFadeT + dt / ctrl->mBgFadeDuration);
                const float t = ctrl->mBgFadeT;
                const float s = SmoothStep01(t);
                bgSp->mColorTint.w = ctrl->mBgTargetAlpha * s;    // 알파 0 -> 목표 알파값
                const float scale = 1.05f + (1.0f - 1.05f) * s;   // 1.05 -> 1.0 줌인
                bgTr->mScale = Vec2(scale, scale);
            }
        }

        if (!ctrl->mHasPending) continue;

        // 상태 전환 적용
        const MainMenuState prev = ctrl->mState;
        ctrl->mState      = ctrl->mPendingState;
        ctrl->mHasPending = false;

        if (prev != ctrl->mState)
        {
            SetEntitiesVisible(ctrl->mStateEntities[(size_t)prev], false);
            SetEntitiesVisible(ctrl->mStateEntities[(size_t)ctrl->mState], true);

            // 이전 배경 : 즉시 숨김
            if (Entity pb = ctrl->mStateBackground[(size_t)prev]; pb.IsValid())
            {
                if (auto* sp = mWorld->GetComponent<UISpriteComponent>(pb))
                {
                    sp->mVisible = false;
                    sp->mColorTint.w = 0.f;
                }
            }

            // 새 배경: 전환 완료 후 페이드인
            ctrl->mActiveBg = ctrl->mStateBackground[(size_t)ctrl->mState];
            ctrl->mBgFadeT  = 0.f;
            if (ctrl->mActiveBg.IsValid())
            {
                if (auto* sp = mWorld->GetComponent<UISpriteComponent>(ctrl->mActiveBg))
                {
                    sp->mVisible = false;
                    sp->mColorTint.w = 0.f;
                }
                if (auto* btr = mWorld->GetComponent<UITransformComponent>(ctrl->mActiveBg))
                    btr->mScale = Vec2(1.05f, 1.05f);
            }
        }

        // 카메라 view 전환 요청
        if (cam->mLoaded)
        {
            Quaternion curQ = Quaternion::CreateFromYawPitchRoll(
                XMConvertToRadians(tr->mLocalRotationE.y),
                XMConvertToRadians(tr->mLocalRotationE.x),
                XMConvertToRadians(tr->mLocalRotationE.z));
            cam->RequestView(
                static_cast<MainMenuView>(static_cast<int>(ctrl->mState)),
                tr->mLocalPosition, curQ,
                XMConvertToDegrees(cc->mFov * 2.f));
        }
    }
}
