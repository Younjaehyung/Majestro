#pragma once
#include "UIComponent.h"
#include "UIFeature.h"

class World;
class CameraComponent;
class UIHpBarComponent;

// HP 바 셰이더에 전달하는 16 DWORD 루트 상수 레이아웃
struct UIHpBarParamsLayout
{
    uint32 BaseInstanceID = 0;
    uint32 PassFlags = 0;
    uint32 SpriteRole = 0;
    uint32 ReservedHeader = 0;

    float AnchorWorldX = 0.f;
    float AnchorWorldY = 0.f;
    float AnchorWorldZ = 0.f;
    float FollowRatio = 1.f;

    float SizePxX = 0.f;
    float SizePxY = 0.f;
    float PivotPxX = 0.f;
    float PivotPxY = 0.f;

    uint32 BackgroundTextureIndex = 0;
    uint32 FillTextureIndex = 0;
    uint32 HitTextureIndex = 0;
    uint32 HitConfig = 0;
};
static_assert(sizeof(UIHpBarParamsLayout) == 16 * 4, "UIHpBarParamsLayout must be 16 DWORDs");

// 파편 1삼각형당 1 인스턴스 (UIInstanceData에 V0/V1/V2/alpha 패킹)


class UIHpBarUpdateFeature : public UIFeature
{
private:
    struct UIInstanceRanges
    {
        uint32 FragmentStart = 0;
        uint32 FragmentCount = 0;
        uint32 HitStart = 0;
        uint32 HitCount = 0;
    };

public:
    void Update(float dt) override;

    void WorldRender(CameraComponent* camera);
    void SpriteRender(DirectX::SpriteBatch* /*spriteBatch*/) {}
    void PostSpriteRender(std::vector<UIInstanceData>& /*instances*/);

private:
    void UpdateHpBarUI(float dt);
    void SpawnHpLossFragments(UIHpBarComponent* hpBar, float oldRatio, float newRatio);
    void UpdateHpLossFragments(UIHpBarComponent* hpBar, float dt);

    void DrawHpBar(UIHpBarComponent* hpBar, Entity owner);
    void DrawUI(CameraComponent* camera, WorldUIPassMode mode = WorldUIPassMode::All);
    UIInstanceRanges UploadBarInstances(const UIHpBarComponent* hpBar);
private:
    uint32 kUIInfoCapacity = 2048;

    UIHpLossFragment mFragment;
};
