#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "UITransformComponent.h"
#include <memory>
#include <vector>

class UIFeature;
class UICommonUpdateFeature;

class UIInputSystem
{
public:
    /*void Update(UIRegistry& registry, const Vec2& mousePos, bool mouseDown)
    {
        for (auto e : registry.View<UITransformComponent, UIButtonComponent>())
        {
            auto& tr = registry.Get<UITransformComponent>(e);
            auto& btn = registry.Get<UIButtonComponent>(e);

            bool inside =
                mousePos.x >= tr.finalPixelPos.x &&
                mousePos.x <= tr.finalPixelPos.x + tr.size.x &&
                mousePos.y >= tr.finalPixelPos.y &&
                mousePos.y <= tr.finalPixelPos.y + tr.size.y;

            btn.hovered = inside;

            if (inside && mouseDown && !btn.pressed)
            {
                btn.pressed = true;
                if (btn.onClick) btn.onClick();
            }

            if (!mouseDown)
                btn.pressed = false;
        }
    }*/
};


class UITransformSystem : public System
{
public:
    UITransformSystem(World* world);
    void Initialize();
    void Update(float dt);

private:
    Vec2 CalculateAnchor(Anchor anchor, const Vec2& screen);
};


class UIUpdateSystem : public System
{
public:
    UIUpdateSystem(World* world);
    void Initialize();
    void Update(float dt);
    void SetFeatures(std::vector<shared_ptr<UIFeature>>* features);

    std::vector<std::type_index> After() const override
    {
        return { std::type_index(typeid(UITransformSystem)) };
    }
private:
	std::vector<std::shared_ptr<UIFeature>>* mFeatures;
	std::shared_ptr<UICommonUpdateFeature> mCommonModule;
};
