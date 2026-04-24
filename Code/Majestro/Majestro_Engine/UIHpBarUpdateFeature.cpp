#include "pch.h"
#include "UIHpBarUpdateFeature.h"

#include "Engine.h"
#include "CameraComponent.h"
#include "HealthComponent.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "TransformComponent.h"
#include "TagComponent.h"
#include "UIComponent.h"
#include "UISpriteComponent.h"
#include "UITransformComponent.h"

void UIHpBarUpdateFeature::Update(float dt)
{
    UpdateHpBarUI(dt);
}

void UIHpBarUpdateFeature::EnsureHpBarUIEntities(UIHpBarComponent* hpBar)
{
    if (hpBar == nullptr)
        return;

    shared_ptr<Texture> hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mBackgroundMaterialName);
    shared_ptr<Texture> hpBarFillTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mFillMaterialName);
    if (hpBarBackgroundTexture == nullptr)
        hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(L"HPBAR");
    if (hpBarFillTexture == nullptr)
        hpBarFillTexture = hpBarBackgroundTexture;

    if (hpBarBackgroundTexture == nullptr || hpBarFillTexture == nullptr)
        return;

    if (hpBar->mBackgroundUIEntity == NULL_ENTITY)
    {
        Entity background = mWorld->CreateEntity();
        UITransformComponent& transform = mWorld->AddComponent<UITransformComponent>(background);
        transform.mAnchor = Anchor::TopLeft;
        transform.mPivot = Vec2(0.f, 0.f);
        transform.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        transform.mUILayerIndex = 10;

        mWorld->AddComponent<UISpriteComponent>(background, hpBarBackgroundTexture);
        hpBar->mBackgroundUIEntity = background;
    }
    else
    {
        UISpriteComponent* backgroundSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (backgroundSprite != nullptr)
            backgroundSprite->mTexture = hpBarBackgroundTexture;
    }

    if (hpBar->mFillUIEntity == NULL_ENTITY)
    {
        Entity fill = mWorld->CreateEntity();
        UITransformComponent& transform = mWorld->AddComponent<UITransformComponent>(fill);
        transform.mAnchor = Anchor::TopLeft;
        transform.mPivot = Vec2(0.f, 0.f);
        transform.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        transform.mUILayerIndex = 11;

        mWorld->AddComponent<UISpriteComponent>(fill, hpBarFillTexture);
        hpBar->mFillUIEntity = fill;
    }
    else
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite != nullptr)
            fillSprite->mTexture = hpBarFillTexture;
    }
}

void UIHpBarUpdateFeature::SetHpBarVisibility(UIHpBarComponent* hpBar, bool visible)
{
    if (hpBar == nullptr)
        return;

    if (hpBar->mBackgroundUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* backgroundSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (backgroundSprite != nullptr)
            backgroundSprite->mVisible = visible;
    }

    if (hpBar->mFillUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite != nullptr)
            fillSprite->mVisible = visible;

        UICusSpriteComponent* fillCustomSprite = mWorld->GetComponent<UICusSpriteComponent>(hpBar->mFillUIEntity);
        if (fillCustomSprite != nullptr)
            fillCustomSprite->mVisible = visible;
    }
}

void UIHpBarUpdateFeature::SpawnHpLossFragments(
    UIHpBarComponent* hpBar,
    const UISpriteComponent* fillSprite,
    const UITransformComponent* fillTransform,
    float oldRatio,
    float newRatio)
{
    if (hpBar == nullptr || fillSprite == nullptr || fillTransform == nullptr || fillSprite->mTexture == nullptr)
        return;

    const float clampedOld = std::clamp(oldRatio, 0.f, 1.f);
    const float clampedNew = std::clamp(newRatio, 0.f, 1.f);
    if (clampedNew >= clampedOld)
        return;

    const float lostScreenStart = hpBar->mMaxWidth * clampedNew;
    const float lostScreenEnd = hpBar->mMaxWidth * clampedOld;
    const float lostScreenWidth = lostScreenEnd - lostScreenStart;
    if (lostScreenWidth <= 1.f)
        return;

    const float textureWidth = static_cast<float>(fillSprite->mTexture->GetWidth());
    const float textureHeight = static_cast<float>(fillSprite->mTexture->GetHeight());
    if (textureWidth <= 0.f || textureHeight <= 0.f || hpBar->mHeight <= 0.f)
        return;

    const float lostUvStart = clampedNew;
    const float lostUvEnd = clampedOld;

    const int fragmentCount = std::clamp(static_cast<int>(lostScreenWidth / 18.f), 3, 10);
    if (static_cast<int>(hpBar->mLossFragments.size()) + fragmentCount > hpBar->mMaxFragmentCount)
    {
        const int removeCount = static_cast<int>(hpBar->mLossFragments.size()) + fragmentCount - hpBar->mMaxFragmentCount;
        hpBar->mLossFragments.erase(
            hpBar->mLossFragments.begin(),
            hpBar->mLossFragments.begin() + std::min<int>(removeCount, static_cast<int>(hpBar->mLossFragments.size())));
    }

    for (int i = 0; i < fragmentCount; ++i)
    {
        const float segmentStartT = static_cast<float>(i) / static_cast<float>(fragmentCount);
        const float segmentEndT = static_cast<float>(i + 1) / static_cast<float>(fragmentCount);

        const float localStart = std::lerp(lostScreenStart, lostScreenEnd, segmentStartT);
        const float localEnd = std::lerp(lostScreenStart, lostScreenEnd, segmentEndT);
        const float baseWidth = std::max(localEnd - localStart, 2.f);

        UIHpLossFragment fragment{};
        fragment.Size.x = std::max(4.f, baseWidth * RandomRange(0.75f, 1.2f));
        fragment.Size.y = std::max(4.f, hpBar->mHeight * RandomRange(0.35f, 1.0f));
        fragment.Center.x = fillTransform->mFinalPixelPos.x + (localStart + localEnd) * 0.5f;
        fragment.Center.y = fillTransform->mFinalPixelPos.y + RandomRange(fragment.Size.y * 0.5f, hpBar->mHeight - fragment.Size.y * 0.5f);

        const float srcStart = std::lerp(lostUvStart, lostUvEnd, segmentStartT) * textureWidth;
        const float srcEnd = std::lerp(lostUvStart, lostUvEnd, segmentEndT) * textureWidth;
        const float srcHeight = textureHeight * (fragment.Size.y / hpBar->mHeight);
        const float srcTop = RandomRange(0.f, std::max(0.f, textureHeight - srcHeight));

        fragment.SourceRect.left = static_cast<LONG>(std::floor(srcStart));
        fragment.SourceRect.right = static_cast<LONG>(std::ceil(std::max(srcEnd, srcStart + 1.f)));
        fragment.SourceRect.top = static_cast<LONG>(std::floor(srcTop));
        fragment.SourceRect.bottom = static_cast<LONG>(std::ceil(std::min(textureHeight, srcTop + srcHeight)));

        fragment.Rotation = RandomRange(-0.18f, 0.18f);
        fragment.AngularVelocity = RandomRange(-6.f, 6.f);
        fragment.Velocity = Vec2(RandomRange(-60.f, 110.f), RandomRange(-130.f, -40.f));
        fragment.LifeTime = hpBar->mFragmentLifeTime * RandomRange(0.85f, 1.15f);

        hpBar->mLossFragments.push_back(fragment);
    }
}

void UIHpBarUpdateFeature::UpdateHpLossFragments(UIHpBarComponent* hpBar, float dt)
{
    if (hpBar == nullptr || hpBar->mLossFragments.empty())
        return;

    constexpr float gravity = 720.f;

    for (UIHpLossFragment& fragment : hpBar->mLossFragments)
    {
        fragment.Age += dt;
        fragment.Center += fragment.Velocity * dt;
        fragment.Velocity.y += gravity * dt;
        fragment.Rotation += fragment.AngularVelocity * dt;
    }

    hpBar->mLossFragments.erase(
        std::remove_if(hpBar->mLossFragments.begin(), hpBar->mLossFragments.end(),
            [](const UIHpLossFragment& fragment)
            {
                return fragment.Age >= fragment.LifeTime;
            }),
        hpBar->mLossFragments.end());
}

void UIHpBarUpdateFeature::UpdateHpBarUI(float dt)
{
    if (mWorld->HasComponentPool<UIHpBarComponent>() == false ||
        mWorld->HasComponentPool<HealthComponent>() == false ||
        mWorld->HasComponentPool<TransformComponent>() == false)
    {
        return;
    }

    bool hasCamera = false;
    Matrix viewProj{};
    const WindowInfo window = RENDERMANAGER.GetWindow();

    if (mWorld->HasComponentPool<MainCameraComponent>() && mWorld->HasComponentPool<CameraComponent>())
    {
        std::vector<Entity> cameras{ mWorld->GetEntitiesWithComponents<MainCameraComponent, CameraComponent>() };
        if (cameras.empty() == false)
        {
            CameraComponent* camera = mWorld->GetComponent<CameraComponent>(cameras[0]);
            if (camera != nullptr)
            {
                viewProj = camera->mView * camera->mProjection;
                hasCamera = true;
            }
        }
    }

    for (Entity owner : mWorld->GetEntitiesWithComponents<UIHpBarComponent, HealthComponent, TransformComponent>())
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(owner);
        if (hpBar == nullptr)
            continue;

        if (hpBar->mTargetEntity == NULL_ENTITY)
            hpBar->mTargetEntity = owner;

        // HP bar spawning, placement, and loss-fragment simulation share one lifecycle,
        // so the whole feature stays in a single module.
        EnsureHpBarUIEntities(hpBar);

        TransformComponent* followTransform = mWorld->GetComponent<TransformComponent>(hpBar->mTargetEntity);
        HealthComponent* followHealth = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);
        UITransformComponent* backgroundTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mBackgroundUIEntity);
        UITransformComponent* fillTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mFillUIEntity);
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);

        if (hasCamera == false || followTransform == nullptr || followHealth == nullptr ||
            backgroundTransform == nullptr || fillTransform == nullptr || fillSprite == nullptr)
        {
            hpBar->mLossFragments.clear();
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const Vec3 worldPos = followTransform->mLocalPosition + hpBar->mWorldOffset;
        const Vec3 ndc = Vec3::Transform(worldPos, viewProj);
        if (ndc.z < 0.0f || ndc.z > 1.0f)
        {
            hpBar->mLossFragments.clear();
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const float pixelX = (ndc.x * 0.5f + 0.5f) * static_cast<float>(window.Width);
        const float pixelY = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(window.Height);
        const float followRatio = std::clamp(static_cast<float>(followHealth->mCurrentHp) / static_cast<float>((std::max)(1, followHealth->mMaxHp)), 0.0f, 1.0f);

        backgroundTransform->mAnchor = Anchor::TopLeft;
        backgroundTransform->mPosition = Vec2(pixelX - hpBar->mMaxWidth * 0.5f, pixelY);
        backgroundTransform->mFinalPixelPos = backgroundTransform->mPosition;
        backgroundTransform->mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        backgroundTransform->mFinalSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);

        fillTransform->mAnchor = Anchor::TopLeft;
        fillTransform->mPosition = backgroundTransform->mPosition;
        fillTransform->mFinalPixelPos = fillTransform->mPosition;
        fillTransform->mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        fillTransform->mFinalSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);

        // Shrinking the destination width distorts the texture.
        // Keep the body cropped via visible range and emit fragments only for the lost range.
        fillSprite->SetVisibleRangeNormalized(0.f, followRatio);
        fillSprite->SetVisibleRangeKeepDestinationSize(false);

        if (hpBar->mHasPreviousHpRatio == false)
        {
            hpBar->mHasPreviousHpRatio = true;
            hpBar->mPreviousHpRatio = followRatio;
        }
        else if (followRatio < hpBar->mPreviousHpRatio - 0.001f)
        {
            SpawnHpLossFragments(hpBar, fillSprite, fillTransform, hpBar->mPreviousHpRatio, followRatio);
            hpBar->mPreviousHpRatio = followRatio;
        }
        else
        {
            hpBar->mPreviousHpRatio = followRatio;
        }

        UpdateHpLossFragments(hpBar, dt);
        SetHpBarVisibility(hpBar, true);
    }
}

void UIHpBarUpdateFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{
    if (spriteBatch == nullptr || mWorld == nullptr)
        return;
    if (mWorld->HasComponentPool<UIHpBarComponent>() == false)
        return;

    for (Entity entity : mWorld->GetEntitiesWithComponent<UIHpBarComponent>())
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(entity);
        if (hpBar == nullptr || hpBar->mLossFragments.empty())
            continue;

        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite == nullptr || fillSprite->mTexture == nullptr)
            continue;

        DrawFragments(spriteBatch, hpBar, fillSprite);
    }
}

void UIHpBarUpdateFeature::DrawFragments(DirectX::SpriteBatch* spriteBatch, const UIHpBarComponent* hpBar, const UISpriteComponent* fillSprite) const
{
    const XMUINT2 textureSize(
        static_cast<uint32_t>(fillSprite->mTexture->GetWidth()),
        static_cast<uint32_t>(fillSprite->mTexture->GetHeight()));

    for (const UIHpLossFragment& fragment : hpBar->mLossFragments)
    {
        const float t = std::clamp(fragment.Age / std::max(fragment.LifeTime, 0.0001f), 0.f, 1.f);
        const float alpha = 1.f - t;
        const float shrink = std::max(0.15f, 1.f - t);
        if (alpha <= 0.01f)
            continue;

        const float srcWidth = static_cast<float>(std::max<LONG>(1, fragment.SourceRect.right - fragment.SourceRect.left));
        const float srcHeight = static_cast<float>(std::max<LONG>(1, fragment.SourceRect.bottom - fragment.SourceRect.top));

        XMFLOAT2 origin(srcWidth * 0.5f, srcHeight * 0.5f);
        XMFLOAT2 scale(
            (fragment.Size.x / srcWidth) * shrink,
            (fragment.Size.y / srcHeight) * shrink);

        const XMVECTOR color = XMVectorSet(1.f, 1.f, 1.f, alpha);

        spriteBatch->Draw(
            fillSprite->mTexture->GetSrvGpuHandle(),
            textureSize,
            fragment.Center,
            &fragment.SourceRect,
            color,
            fragment.Rotation,
            origin,
            scale);
    }
}
