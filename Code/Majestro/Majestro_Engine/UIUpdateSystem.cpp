#include "pch.h"
#include "UIUpdateSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "HealthComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"

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

   UpdateHpBarUI();

}

void UIUpdateSystem::UpdateSpriteAnimation(float dt)
{
    if (false == mWorld->HasComponentPool<UISpriteComponent>())return;
    
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UISpriteComponent>() };

    WindowInfo window = RENDERMANAGER.GetWindow();

    Vec2 screenSize = { (float)window.Width ,(float)window.Height };
    for (auto& e : entitys)
    {
        UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e);
        if (false == sp->mIsAnimated)
            continue;
        if (sp->mAnimationLoopTime <= 0.f)
            continue;

		sp->mAnimationUpdateTime += dt;

        sp->mAnimationUpdateTime = std::fmod(sp->mAnimationUpdateTime * sp->mAnimationSpeed, sp->mAnimationLoopTime);

        if (!sp->mTextures.empty())
        {
            const float progress = sp->mAnimationUpdateTime / sp->mAnimationLoopTime;
            const size_t frameIndex = static_cast<size_t>(progress * sp->mTextures.size()) % sp->mTextures.size();
            sp->mTexture = sp->mTextures[frameIndex];
        }
        else if (sp->mFrameCount > 1)
        {
            const float progress = sp->mAnimationUpdateTime / sp->mAnimationLoopTime;
            const int frameIndex = static_cast<int>(progress * sp->mFrameCount) % sp->mFrameCount;
            sp->SetCurrentFrame(frameIndex);
        }

    }

}


void UIUpdateSystem::EnsureHpBarUIEntities(UIHpBarComponent* hpBar)
{
    if (!hpBar)
        return;

    shared_ptr<Texture> hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mBackgroundMaterialName);
    shared_ptr<Texture> hpBarFillTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mFillMaterialName);
    if (!hpBarBackgroundTexture)
        hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(L"HPBAR");
    if (!hpBarFillTexture)
        hpBarFillTexture = hpBarBackgroundTexture;

    if (!hpBarBackgroundTexture || !hpBarFillTexture)
        return;

    if (hpBar->mBackgroundUIEntity == NULL_ENTITY)
    {
        Entity background = mWorld->CreateEntity();
        auto& tr = mWorld->AddComponent<UITransformComponent>(background);
        tr.mAnchor = Anchor::TopLeft;
        tr.mPivot = Vec2(0.f, 0.f);
        tr.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        tr.mUILayerIndex = 10;

        mWorld->AddComponent<UISpriteComponent>(background, hpBarBackgroundTexture);
        hpBar->mBackgroundUIEntity = background;
    }
    else
    {
        UISpriteComponent* bgSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (bgSprite)
            bgSprite->mTexture = hpBarBackgroundTexture;
    }

    if (hpBar->mFillUIEntity == NULL_ENTITY)
    {
        Entity fill = mWorld->CreateEntity();
        auto& tr = mWorld->AddComponent<UITransformComponent>(fill);
        tr.mAnchor = Anchor::TopLeft;
        tr.mPivot = Vec2(0.f, 0.f);
        tr.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        tr.mUILayerIndex = 11;

        mWorld->AddComponent<UISpriteComponent>(fill, hpBarFillTexture);
        hpBar->mFillUIEntity = fill;
    }
    else
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite)
            fillSprite->mTexture = hpBarFillTexture;
    }
}

void UIUpdateSystem::SetHpBarVisibility(UIHpBarComponent* hpBar, bool visible)
{
    if (!hpBar)
        return;

    if (hpBar->mBackgroundUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* bgSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (bgSprite)
            bgSprite->mVisible = visible;
    }

    if (hpBar->mFillUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite)
            fillSprite->mVisible = visible;

        UICusSpriteComponent* fillCusSprite = mWorld->GetComponent<UICusSpriteComponent>(hpBar->mFillUIEntity);
        if (fillCusSprite)
            fillCusSprite->mVisible = visible;
    }
}

void UIUpdateSystem::UpdateHpBarUI()
{
    if (!mWorld->HasComponentPool<UIHpBarComponent>() ||
        !mWorld->HasComponentPool<HealthComponent>() ||
        !mWorld->HasComponentPool<TransformComponent>())
    {
        return;
    }

    bool hasCamera = false;
    Matrix viewProj{};
    WindowInfo window = RENDERMANAGER.GetWindow();

    if (mWorld->HasComponentPool<MainCameraComponent>() && mWorld->HasComponentPool<CameraComponent>())
    {
        std::vector<Entity> cameras{ mWorld->GetEntitiesWithComponents<MainCameraComponent, CameraComponent>() };
        if (!cameras.empty())
        {
            CameraComponent* cam = mWorld->GetComponent<CameraComponent>(cameras[0]);
            if (cam)
            {
                viewProj = cam->mView * cam->mProjection;
                hasCamera = true;
            }
        }
    }

    std::vector<Entity> owners{ mWorld->GetEntitiesWithComponents<UIHpBarComponent, HealthComponent, TransformComponent>() };
    for (Entity owner : owners)
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(owner);
        if (!hpBar)
            continue;

        if (hpBar->mTargetEntity == NULL_ENTITY)
            hpBar->mTargetEntity = owner;

        EnsureHpBarUIEntities(hpBar);

        TransformComponent* followTransform = mWorld->GetComponent<TransformComponent>(hpBar->mTargetEntity);
        HealthComponent* followHealth = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);

        UITransformComponent* bgTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mBackgroundUIEntity);
        UITransformComponent* fillTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mFillUIEntity);
        if (!hasCamera || !followTransform || !followHealth || !bgTransform || !fillTransform)
        {
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const Vec3 worldPos = followTransform->mLocalPosition + hpBar->mWorldOffset;
        const Vec3 ndc = Vec3::Transform(worldPos, viewProj);

        if (ndc.z < 0.0f || ndc.z > 1.0f)
        {
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const float pixelX = (ndc.x * 0.5f + 0.5f) * static_cast<float>(window.Width);
        const float pixelY = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(window.Height);

        bgTransform->mAnchor = Anchor::TopLeft;
        bgTransform->mPosition = Vec2(pixelX - hpBar->mMaxWidth * 0.5f, pixelY);
        bgTransform->mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);

        fillTransform->mAnchor = Anchor::TopLeft;
        fillTransform->mPosition = bgTransform->mPosition;
        const float followRatio = std::clamp(static_cast<float>(followHealth->mCurrentHp) / static_cast<float>((std::max)(1, followHealth->mMaxHp)), 0.0f, 1.0f);
        fillTransform->mSize = Vec2(hpBar->mMaxWidth * followRatio, hpBar->mHeight);

        SetHpBarVisibility(hpBar, true);
    }
}