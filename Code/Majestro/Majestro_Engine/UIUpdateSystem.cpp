#include "pch.h"
#include "UIUpdateSystem.h"

#include "Engine.h"
#include "RenderManager.h"
#include "UIActionUpdateFeature.h"
#include "UIAudioVisualizerFeature.h"
#include "UICommonUpdateFeature.h"
#include "UIHpBarUpdateFeature.h"
#include "UIFeature.h"

UITransformSystem::UITransformSystem(World* world) : System::System(world)
{
}

void UITransformSystem::Initialize()
{
}

void UITransformSystem::Update(float /*dt*/)
{
    if (mWorld->HasComponentPool<UITransformComponent>() == false)
        return;

    std::vector<Entity> entities{ mWorld->GetEntitiesWithComponent<UITransformComponent>() };

    const WindowInfo window = RENDERMANAGER.GetWindow();
    const Vec2 screenSize = { static_cast<float>(window.Width), static_cast<float>(window.Height) };
    for (Entity e : entities)
    {
        UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(e);
        if (transform == nullptr)
            continue;

        const Vec2 anchorBase = CalculateAnchor(transform->mAnchor, screenSize);
        transform->mFinalPixelPos = anchorBase + transform->mPosition;
        transform->mFinalSize = transform->mSize;
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

void UIUpdateSystem::SetFeatures(std::vector<shared_ptr<UIFeature>>* features)
{ 
    mCommonModule = std::make_shared<UICommonUpdateFeature>();
    mCommonModule->Initialize(mWorld);
    mFeatures = features;
}

void UIUpdateSystem::Update(float dt)
{
	mCommonModule->Update(dt);

    if (mFeatures == nullptr)
		return;

    for (const auto& feature : *mFeatures)
    {
        if (feature != nullptr)
            feature->Update(dt);
    }
}
