#include "pch.h"
#include "UIActionUpdateFeature.h"

#include "MathUtils.h"
#include "GameEvents.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "Engine.h"
#include "RenderManager.h"

void UIActionUpdateFeature::Update(float dt)
{
    UpdateActiveUIEntities(dt);
}

void UIActionUpdateFeature::UpdateActiveUIEntities(float dt)
{
    if (mWorld->HasComponentPool<UIActionComponent>() == false)
        return;

    bool beatFired = false;
    mWorld->GetEventManager()->Consume<EvBeat>([&](const EvBeat&) {
        beatFired = true;
    });

    for (Entity e : mWorld->GetEntitiesWithComponent<UIActionComponent>())
    {
        UIActionComponent* uiAction = mWorld->GetComponent<UIActionComponent>(e);
        UITransformComponent* uiTransform = mWorld->GetComponent<UITransformComponent>(e);
        if (uiAction == nullptr || uiTransform == nullptr)
            continue;

        const WindowInfo& window = RENDERMANAGER.GetWindow();
        const Vec2 screenSize = { static_cast<float>(window.Width), static_cast<float>(window.Height) };
        Vec2 baseSize = uiTransform->mSize;
        if (uiTransform->mLayoutMode == UILayoutMode::ScreenRatio)
        {
            baseSize = Vec2(uiTransform->mSizeRatio.x * screenSize.x,
                            uiTransform->mSizeRatio.y * screenSize.y);
        }
        else if (uiTransform->mLayoutMode == UILayoutMode::ReferenceResolution)
        {
            const Vec2 reference = uiTransform->mReferenceResolution;
            if (reference.x > 0.f && reference.y > 0.f)
            {
                baseSize = Vec2(uiTransform->mSize.x * (screenSize.x / reference.x),
                                uiTransform->mSize.y * (screenSize.y / reference.y));
            }
        }

        if (beatFired && uiAction->mState == UIActionState::Bounce)
        {
            uiAction->mElapsedTime = 0.f;
            uiAction->mIsActive = true;
        }

        if (uiAction->mIsActive == false)
            continue;

        uiAction->mElapsedTime += dt;

        if (uiAction->mOnCustomAction)
        {
            uiAction->mOnCustomAction();
            continue;
        }

        if (uiAction->mState == UIActionState::Vibration)
        {
            uiTransform->mFinalPixelPos += Vec2(
                std::sin(uiAction->mElapsedTime * uiAction->mVibrationFrequency * 2.f * kPI) * uiAction->mVibrationAmplitude,
                std::cos(uiAction->mElapsedTime * uiAction->mVibrationFrequency * 2.f * kPI) * uiAction->mVibrationAmplitude);
        }
        else if (uiAction->mState == UIActionState::Hovered)
        {
            const float progress = std::clamp(uiAction->mElapsedTime / uiAction->mDuration, 0.f, 1.f);
            uiTransform->mFinalSize = baseSize * (uiAction->mDefaultScale + (uiAction->mHoverScale - uiAction->mDefaultScale) * progress);
        }
        else if (uiAction->mState == UIActionState::Bounce)
        {
            const float progress = std::clamp(uiAction->mElapsedTime / uiAction->mDuration, 0.f, 1.f);
            const float bounce = std::sin(progress * kPI) * uiAction->mBounceAmplitude;
            uiTransform->mFinalSize = baseSize * (1.f + bounce);
        }

        if (uiAction->mElapsedTime >= uiAction->mDuration)
        {
            if (uiAction->mIsLoop == false)
                uiAction->mIsActive = false;

            uiAction->mElapsedTime = 0.f;
        }
    }
}
