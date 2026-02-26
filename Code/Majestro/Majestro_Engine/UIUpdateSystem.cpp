#include "pch.h"
#include "UIUpdateSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"

UITransformSystem::UITransformSystem(World* world) : System::System(world)
{
}

void UITransformSystem::Initialize()
{
}

void UITransformSystem::Update(float dt)
{
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITransformComponent>() };

	WindowInfo window = RENDERMANAGER.GetWindow();

    Vec2 screenSize={(float) window.Width ,(float)window.Height };
    for (auto& e : entitys)
    {
        auto tr = mWorld->GetComponent<UITransformComponent>(e);
       
		
        Vec2 anchorBase = CalculateAnchor(tr->mAnchor, screenSize);
        tr->mFinalPixelPos = anchorBase + tr->mPosition;
    }
}

Vec2 UITransformSystem::CalculateAnchor(Anchor anchor, const Vec2& screen)
{
    switch (anchor)
    {
    case Anchor::TopLeft:     return { 0, 0 };
    case Anchor::TopRight:    return { screen.x, 0 };
    case Anchor::BottomLeft:  return { 0, screen.y };
    case Anchor::BottomRight: return { screen.x, screen.y };
    case Anchor::Center:      return { screen.x * 0.5f, screen.y * 0.5f };
    }
    return { 0, 0 };
}


UIUpdateSystem::UIUpdateSystem(World* world) : System::System(world)
{
}

void UIUpdateSystem::Initialize()
{
}

void UIUpdateSystem::Update(float dt)
{
    UpdateSpriteAnimation(dt);

   

}

void UIUpdateSystem::UpdateSpriteAnimation(float dt)
{
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UISpriteComponent>() };

    WindowInfo window = RENDERMANAGER.GetWindow();

    Vec2 screenSize = { (float)window.Width ,(float)window.Height };
    for (auto& e : entitys)
    {
        UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e);
        if (false == sp->mIsAnimated)
            continue;

		sp->mAnimationUpdateTime += dt;

		sp->mAnimationUpdateTime = (sp->mAnimationUpdateTime >= sp->mAnimationLoopTime) ? 0.f : sp->mAnimationUpdateTime;
        
		sp->mTexture = sp->mTextures[static_cast<size_t>((sp->mAnimationUpdateTime / sp->mAnimationLoopTime) * sp->mTextures.size())];

    }

}
