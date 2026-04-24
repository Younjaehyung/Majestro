#pragma once
#include "World.h"
#include "UIRenderSystem.h"

class UIFeature
{
public:
    virtual ~UIFeature() = default;

    virtual void Initialize(World* world)
    {
        mWorld = world;
    }

    virtual void Update(float dt) {};

    virtual void SpriteRender(DirectX::SpriteBatch* spriteBatch) {};

    virtual void CustomSpriteRender(std::vector<UIInstanceData>& instances) {};

protected:
    World* mWorld = nullptr;
};