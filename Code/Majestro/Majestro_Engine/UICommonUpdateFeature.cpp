#include "pch.h"
#include "UICommonUpdateFeature.h"

#include "UIComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"

void UICommonUpdateFeature::Update(float dt)
{
    UpdateScripts(dt);
    UpdateSpriteAnimation(dt);
    UpdateTextContext(dt);
}

void UICommonUpdateFeature::UpdateScripts(float dt)
{
    if (mWorld->HasComponentPool<UIScriptComponent>() == false)
        return;

    for (Entity e : mWorld->GetEntitiesWithComponent<UIScriptComponent>())
    {
        UIScriptComponent* script = mWorld->GetComponent<UIScriptComponent>(e);
        if (script != nullptr && script->mOnUpdate)
            script->mOnUpdate(dt);
    }
}

void UICommonUpdateFeature::UpdateSpriteAnimation(float dt)
{
    if (mWorld->HasComponentPool<UISpriteComponent>() == false)
        return;

    for (Entity e : mWorld->GetEntitiesWithComponent<UISpriteComponent>())
    {
        UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(e);
        if (sprite == nullptr || sprite->mIsAnimated == false || sprite->mAnimationLoopTime <= 0.f)
            continue;

        sprite->mAnimationUpdateTime += dt;
        sprite->mAnimationUpdateTime = std::fmod(sprite->mAnimationUpdateTime * sprite->mAnimationSpeed, sprite->mAnimationLoopTime);

        if (sprite->mTextures.empty() == false)
        {
            const float progress = sprite->mAnimationUpdateTime / sprite->mAnimationLoopTime;
            const size_t frameIndex = static_cast<size_t>(progress * sprite->mTextures.size()) % sprite->mTextures.size();
            sprite->mTexture = sprite->mTextures[frameIndex];
        }
        else if (sprite->mFrameCount > 1)
        {
            const float progress = sprite->mAnimationUpdateTime / sprite->mAnimationLoopTime;
            const int frameIndex = static_cast<int>(progress * sprite->mFrameCount) % sprite->mFrameCount;
            sprite->SetCurrentFrame(frameIndex);
        }
    }
}

void UICommonUpdateFeature::UpdateTextContext(float /*dt*/)
{
    if (mWorld->HasComponentPool<UITextComponent>() == false)
        return;

    for (Entity e : mWorld->GetEntitiesWithComponent<UITextComponent>())
    {
        UITextComponent* text = mWorld->GetComponent<UITextComponent>(e);
        if (text == nullptr || text->mOnTextChanged == nullptr)
            continue;

        text->mOnTextChanged();
        text->mIsDirty = false;
    }
}
