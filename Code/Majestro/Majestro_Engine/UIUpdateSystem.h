#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "UITransformComponent.h"

class UIHpBarComponent;

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

private:
	void UpdateScripts(float dt);
    void UpdateSpriteAnimation(float dt);
    void UpdateHpBarUI();
    void UpdateAudioVisualizer(float dt);
    void UpdateActiveUIEntities(float dt);
    void EnsureHpBarUIEntities(UIHpBarComponent* hpBar);
    void SetHpBarVisibility(UIHpBarComponent* hpBar, bool visible);
	void UpdateTextContext(float dt);
private:
    // 저주파 에너지 감지에 사용할 포인트 수
    static constexpr int kBassPoints = 6;

    // 내부 처리 대역 수: AudioVisualizerSystem과 동일하게 32개 대역으로 처리해
    // 각 대역이 충분히 넓어야 peak 값이 유의미하게 나온다.
    // 이후 128개 포인트로 선형 보간 업샘플링하여 waveAmplitudes에 저장.
    static constexpr int kInternalBands = 32;
    std::pair<int, int> GetBinRange(int pointIdx, int totalPoints, int spectrumSize, float sampleRate) const;
};

