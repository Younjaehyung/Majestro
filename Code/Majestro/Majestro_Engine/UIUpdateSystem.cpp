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

        Vec2 positionOffset = transform->mPosition;
        Vec2 baseSize = transform->mSize;

        switch (transform->mLayoutMode)
        {
        case UILayoutMode::ScreenRatio:  // 화면 비율 모드 : 저장된 비율값을 현재 화면 픽셀값으로 변환
            positionOffset = Vec2(transform->mPositionRatio.x * screenSize.x,
                                  transform->mPositionRatio.y * screenSize.y);
            baseSize = Vec2(transform->mSizeRatio.x * screenSize.x,
                            transform->mSizeRatio.y * screenSize.y);
            break;
        case UILayoutMode::ReferenceResolution: // 픽셀 비율 모드 : 기존 Prefab 픽셀값을 기준 해상도 대비 현재 해상도 비율로 자동 변환
        {
            const Vec2 reference = transform->mReferenceResolution;
            if (reference.x > 0.f && reference.y > 0.f)
            {
                const Vec2 scale = Vec2(screenSize.x / reference.x, screenSize.y / reference.y);
                positionOffset = Vec2(transform->mPosition.x * scale.x, transform->mPosition.y * scale.y);
                baseSize = Vec2(transform->mSize.x * scale.x, transform->mSize.y * scale.y);
            }
            break;
        }
        case UILayoutMode::Pixel:
        default:
            break;
        }

        transform->mFinalPixelPos = anchorBase + positionOffset;
        transform->mFinalSize = Vec2(baseSize.x * transform->mScale.x,
                                     baseSize.y * transform->mScale.y);
    }
}

Vec2 UITransformSystem::CalculateAnchor(Anchor anchor, const Vec2& screen)
{
    switch (anchor)
    {
    case Anchor::TopLeft:      return { 0, 0 };
    case Anchor::TopCenter:    return { screen.x * 0.5f, 0 };
    case Anchor::TopRight:     return { screen.x, 0 };
    case Anchor::BottomLeft:   return { 0, screen.y };
    case Anchor::BottomCenter: return { screen.x * 0.5f, screen.y };
    case Anchor::BottomRight:  return { screen.x, screen.y };
    case Anchor::Center:       return { screen.x * 0.5f, screen.y * 0.5f };
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
