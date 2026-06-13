#include "pch.h"
#include "UIActionUpdateFeature.h"

#include "MathUtils.h"
#include "GameEvents.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "Engine.h"
#include "RenderManager.h"
#include "BeatSystem.h"


void UIActionUpdateFeature::Update(float dt)
{
    UpdateActiveUIEntities(dt);
}

void UIActionUpdateFeature::UpdateActiveUIEntities(float dt)
{
    if (mWorld->HasComponentPool<UIActionComponent>() == false)
        return;

    float beatProgress = 0.f;   // 0~1, 한 박자 안에서의 위치
    bool  hasBeat = false;
    if (auto systemManager = mWorld->GetSystemManager())
    {
        if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
        {
            const float beatSec = beatSystem->GetBeatSeconds();
            if (beatSec > 0.f)
            {
                beatProgress = std::fmod(beatSystem->GetSongPosition(), beatSec) / beatSec;
                hasBeat = true;
            }
        }
    }

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

        // 박자 바운스
        if (uiAction->mState == UIActionState::Bounce)
        {
            if (hasBeat)
            {
                const float decay = 1.f - beatProgress;
                const float bounce = decay * decay * uiAction->mBounceAmplitude;
                uiTransform->mFinalSize = baseSize * (1.f + bounce);
            }
            continue;
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

        if (uiAction->mElapsedTime >= uiAction->mDuration)
        {
            if (uiAction->mIsLoop == false)
                uiAction->mIsActive = false;

            uiAction->mElapsedTime = 0.f;
        }
    }
}
