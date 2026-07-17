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
#include "UIComponent.h"
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
        if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e))
        {
            // 메뉴 상태가 바뀌어 버튼이 숨겨질 때 남아 있는 호버 오버레이도 즉시 제거한다.
            if (!visible && bt->mHovered && bt->mOnHoverExit)
                bt->mOnHoverExit();
            if (!visible)
            {
                bt->mHovered = false;
                bt->mPressed = false;
            }
            bt->mEnabled = visible;
        }
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

        // 카메라 전환 완료 후 처리
        if (cam->mBlendT >= 1.f)
        {
            // 버튼/텍스트 : 즉시 등장 (1회)
            if (ctrl->mEntitiesPendingReveal)
            {
                SetEntitiesVisible(ctrl->mStates[(size_t)ctrl->mState].entities, true);

                // 상태 표시 직후 내부 탭 제어 스크립트를 실행해 비활성 그룹을 같은 프레임에 숨긴다.
                // 장식 애니메이션
                for (Entity a : ctrl->mStates[(size_t)ctrl->mState].animations)
                {
                    if (auto* sp = mWorld->GetComponent<UISpriteComponent>(a))
                    {
                        sp->mVisible             = true;    // 보이기
                        sp->mAnimationUpdateTime = 0.f;     // 0프레임부터 재생
                        sp->SetCurrentFrame(0);
                    }
                }

                ctrl->mEntitiesPendingReveal = false;

                // 상태 표시 직후 내부 탭 제어 스크립트를 실행해 비활성 그룹을 같은 프레임에 숨긴다.
                for (Entity stateEntity : ctrl->mStates[(size_t)ctrl->mState].entities)
                {
                    if (auto* script = mWorld->GetComponent<UIScriptComponent>(stateEntity))
                        if (script->mOnUpdate)
                            script->mOnUpdate(0.f);
                }
            }

            // 배경
            if (ctrl->mActiveBg.IsValid())
            {
                auto* bgSp = mWorld->GetComponent<UISpriteComponent>(ctrl->mActiveBg);
                auto* bgTr = mWorld->GetComponent<UITransformComponent>(ctrl->mActiveBg);
                if (bgSp && bgTr)
                {
                    bgSp->mVisible = true;
                    ctrl->mBgFadeT = (std::min)(1.f, ctrl->mBgFadeT + dt / ctrl->mBgFadeDuration);
                    const float t = ctrl->mBgFadeT;
                    const float s = MathUtils::SmoothStep01(t);
                    bgSp->mColorTint.w = ctrl->mBgTargetAlpha * s;    // 알파 0 -> 목표 알파값
                    const float scale = 2.05f + (1.0f - 2.05f) * s;   // 1.05 -> 1.0 줌인
                    bgTr->mScale = Vec2(scale, scale);
                }
            }
        }

        if (!ctrl->mHasPending) continue;

        // 상태 전환 적용
        const MainMenuState prev = ctrl->mState;
        ctrl->mState      = ctrl->mPendingState;
        ctrl->mHasPending = false;

        if (prev != ctrl->mState)
        {
            // 이전 상태 UI : 즉시 숨김
            SetEntitiesVisible(ctrl->mStates[(size_t)prev].entities, false);           
            SetEntitiesVisible(ctrl->mStates[(size_t)prev].animations, false);
            // 새 상태 UI : 카메라 전환 완료 후 등장
            SetEntitiesVisible(ctrl->mStates[(size_t)ctrl->mState].entities, false);
            SetEntitiesVisible(ctrl->mStates[(size_t)ctrl->mState].animations, false);
            ctrl->mEntitiesPendingReveal = true;

            // 이전 배경 : 즉시 숨김
            if (Entity pb = ctrl->mStates[(size_t)prev].background; pb.IsValid())
            {
                if (auto* sp = mWorld->GetComponent<UISpriteComponent>(pb))
                {
                    sp->mVisible = false;
                    sp->mColorTint.w = 0.f;
                }
            }
            
            // 새 배경: 전환 완료 후 페이드인
            ctrl->mActiveBg = ctrl->mStates[(size_t)ctrl->mState].background;
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
