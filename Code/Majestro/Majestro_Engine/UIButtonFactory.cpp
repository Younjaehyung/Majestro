#include "pch.h"
#include "UIButtonFactory.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "UISpriteComponent.h"
#include "UIVfxComponent.h"
#include "UITextComponent.h"
#include "UIButtonComponent.h"
#include "Texture.h"
#include "Vfx.h"

Entity CreateUIButton(World* world, const UIButtonDesc& d)
{
    Entity e = world->CreateEntity();

    // 위치, 크기
    auto& tr         = world->AddComponent<UITransformComponent>(e);
    tr.mAnchor       = d.anchor;
    tr.mPosition     = d.position;
    tr.mSize         = d.size;
    tr.mPivot        = d.pivot;
    tr.mUILayerIndex = d.layer;

    // 비주얼 컴포넌트 (종류별 분기)
    switch (d.visual)
    {
    case UIButtonVisual::Texture:
        world->AddComponent<UISpriteComponent>(e, RESOURCEMANAGER.Get<Texture>(d.resKey));
        break;

    case UIButtonVisual::SpriteSheet:
        world->AddComponent<UISpriteComponent>(
            e, RESOURCEMANAGER.Get<Texture>(d.resKey), d.frameSize, d.frameCount, d.animTime);
        break;

    case UIButtonVisual::Vfx:
        world->AddComponent<UIVfxComponent>(
            e, RESOURCEMANAGER.Get<Vfx>(d.resKey), true, d.vfxNormalScale);
        break;
    }

    // 텍스트 레이블 (선택)
    if (d.label)
    {
        auto& txt = world->AddComponent<UITextComponent>(e);
        txt.mText = d.label;
    }

    // 버튼 컴포넌트
    auto& btn     = world->AddComponent<UIButtonComponent>(e);
    btn.mBaseSize = d.size;
    btn.mOnClick  = d.onClick;

    // 일반 크기/색상
    btn.mNormalScale  = d.normalScale;
    btn.mHoveredScale = d.hoveredScale;
    btn.mPressedScale = d.pressedScale;
    btn.mNormalColor  = d.normalColor;
    btn.mHoveredColor = d.hoveredColor;
    btn.mPressedColor = d.pressedColor;

    // VFX 이펙트 스케일
    if (d.visual == UIButtonVisual::Vfx)
    {
        btn.mVfxNormalScale  = d.vfxNormalScale;
        btn.mVfxHoveredScale = d.vfxHoveredScale;
        btn.mVfxPressedScale = d.vfxPressedScale;
    }

    return e;
}
