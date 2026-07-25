#pragma once
#include "World.h"
#include "UIRenderSystem.h"
#include "UIComponent.h"

class CameraComponent;

enum class WorldUIPassMode
{
    World,  // mIsScreenSpace=false 인 바만 그림 (기존 GameRenderPipeline 호출 경로용)
    HUD,    // mIsScreenSpace=true 인 바만 그림 (UIRenderSystem 이후 별도 호출용)
    All,    // 둘 다 (테스트/통합용)
};


class UIFeature
{
public:
    virtual ~UIFeature() = default;

    virtual void Initialize(World* world)
    {
        mWorld = world;
    }

    virtual void Update(float dt) {};

	virtual void WorldRender(CameraComponent* camera) {};

    virtual void SpriteRender(DirectX::SpriteBatch* spriteBatch) {};

    virtual void CustomSpriteRender(std::vector<UIInstanceData>& instances) {};

    virtual void PostSpriteRender(std::vector<UIInstanceData>& instances) {};

    virtual bool RendersInGroup(UIRenderGroup group) const { return group == UIRenderGroup::Gameplay; }

    virtual bool PostRendersInGroup(UIRenderGroup group) const { return RendersInGroup(group); }

protected:
    World* mWorld = nullptr;
};