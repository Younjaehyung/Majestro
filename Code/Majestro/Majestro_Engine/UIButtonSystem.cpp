#include "pch.h"
#include "UIButtonSystem.h"
#include "UIButtonComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "Engine.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "UIUpdateSystem.h"
#include "UIVfxComponent.h"

UIButtonSystem::UIButtonSystem(World* world) : System(world)
{
    mPhase = SysPhase::Post;
}

void UIButtonSystem::Initialize()
{
}

std::vector<std::type_index> UIButtonSystem::After() const
{
    // UITransformSystem이 mFinalPixelPos를 먼저 계산해야 히트테스트 가능
    return { typeid(UIUpdateSystem) };
}

bool UIButtonSystem::HitTest(const Vec2& finalPixelPos, const Vec2& pivot,
                              const Vec2& size, const Vec2& mousePos) const
{
    // mFinalPixelPos는 pivot 기준 위치이므로 좌상단을 역산
    // topLeft = finalPixelPos - pivot * size
    Vec2 topLeft  = finalPixelPos - Vec2(pivot.x * size.x, pivot.y * size.y);
    Vec2 botRight = topLeft + size;

    return mousePos.x >= topLeft.x && mousePos.x <= botRight.x &&
           mousePos.y >= topLeft.y && mousePos.y <= botRight.y;
}

void UIButtonSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<UIButtonComponent>())   return;
    if (!mWorld->HasComponentPool<UITransformComponent>()) return;

    const MouseState& mouse = INPUT.GetMouseState();
    Vec2 mousePos = { static_cast<float>(mouse.Position.x),
                      static_cast<float>(mouse.Position.y) };
    bool leftDown = mouse.LeftDown;
    const WindowInfo& window = RENDERMANAGER.GetWindow();
    const Vec2 screenSize = { static_cast<float>(window.Width), static_cast<float>(window.Height) };

    std::vector<Entity> entities =
        mWorld->GetEntitiesWithComponents<UITransformComponent, UIButtonComponent>();

    for (Entity e : entities)
    {
        UITransformComponent* tr  = mWorld->GetComponent<UITransformComponent>(e);
        UIButtonComponent*    btn = mWorld->GetComponent<UIButtonComponent>(e);
        if (!tr || !btn) continue;

       
        bool inside = HitTest(tr->mFinalPixelPos, tr->mPivot, tr->mFinalSize, mousePos);

        // ── 호버 상태 전환 ──────────────────────────────────────
        if (inside && !btn->mHovered)
        {
            btn->mHovered = true;
            if (btn->mOnHoverEnter) btn->mOnHoverEnter();
        }
        else if (!inside && btn->mHovered)
        {
            btn->mHovered = false;
            if (btn->mOnHoverExit) btn->mOnHoverExit();
        }

        // ── 클릭 감지 (Down 엣지: 버튼 위에서 처음 누르는 순간만 발화) ──
        if (inside && leftDown && !btn->mPressed)
        {
            btn->mPressed = true;
            if (btn->mOnClick) btn->mOnClick();
        }
        if (!leftDown)
            btn->mPressed = false;

        // ── 현재 시각 상태 결정 ────────────────────────────────
        ButtonVisualState curState;
        if      (btn->mPressed && inside) curState = ButtonVisualState::Pressed;
        else if (btn->mHovered)           curState = ButtonVisualState::Hovered;
        else                              curState = ButtonVisualState::Normal;

        // 상태 변화가 없으면 갱신 스킵
        if (curState == btn->mPrevState) continue;
        btn->mPrevState = curState;

        // ── 상태에 따른 색상 / 크기 결정 ──────────────────────
        Vec4  targetColor;
        float targetScale;
        switch (curState)
        {
        case ButtonVisualState::Hovered:
            targetColor = btn->mHoveredColor;
            targetScale = btn->mHoveredScale;
            break;
        case ButtonVisualState::Pressed:
            targetColor = btn->mPressedColor;
            targetScale = btn->mPressedScale;
            break;
        default:
            targetColor = btn->mNormalColor;
            targetScale = btn->mNormalScale;
            break;
        }

        // ── 머티리얼 Diffuse 갱신 ─────────────────────────────
        // RenderSystem::PushMaterialData()가 매 프레임 GPU에 업로드함
        UICusSpriteComponent* sprite = mWorld->GetComponent<UICusSpriteComponent>(e);
        if (sprite && sprite->mMaterial)
            sprite->mMaterial->GetParams().Diffuse = targetColor;

        //// ── 크기 스케일 갱신 (BaseSize 기준) ─────────────────
        //if (btn->mBaseSize.x > 0.f)
        //{
        //    // 버튼 피드백 크기도 UITransformComponent의 레이아웃 모드와 같은 기준으로 계산한다.
        //    Vec2 buttonBaseSize = btn->mBaseSize;
        //    if (tr->mLayoutMode == UILayoutMode::ScreenRatio)
        //    {
        //        buttonBaseSize = Vec2(tr->mSizeRatio.x * screenSize.x,
        //                              tr->mSizeRatio.y * screenSize.y);
        //    }
        //    else if (tr->mLayoutMode == UILayoutMode::ReferenceResolution)
        //    {
        //        const Vec2 reference = tr->mReferenceResolution;
        //        if (reference.x > 0.f && reference.y > 0.f)
        //        {
        //            buttonBaseSize = Vec2(btn->mBaseSize.x * (screenSize.x / reference.x),
        //                                  btn->mBaseSize.y * (screenSize.y / reference.y));
        //        }
        //    }

        //    tr->mFinalSize = buttonBaseSize * targetScale;
        //}

        // ── VFX 스케일 갱신 ───────────────────────────────────
        // UICusSpriteComponent 없이 VFX만 사용하는 버튼도 시각 피드백 지원
        UIVfxComponent* vfx = mWorld->GetComponent<UIVfxComponent>(e);
        if (vfx)
        {
            switch (curState)
            {
            case ButtonVisualState::Hovered: vfx->mScale = btn->mVfxHoveredScale; break;
            case ButtonVisualState::Pressed: vfx->mScale = btn->mVfxPressedScale; break;
            default:                         vfx->mScale = btn->mVfxNormalScale;  break;
            }
        }

        // ── 텍스트 위치 갱신 ──────────────────────────────────
        // UITextComponent의 mFontPos를 버튼 중심 좌표로 설정
        UITextComponent* text = mWorld->GetComponent<UITextComponent>(e);
        if (text)
            text->mFontPos = tr->mFinalPixelPos;
    }
}
