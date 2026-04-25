#pragma once
#include "UIComponent.h"
#include "UIFeature.h"

class World;
class CameraComponent;
class UIHpBarComponent;

struct GlobalParamsLayout
{
    uint32 BaseInstanceID = 0;
    uint32 etc = 0;
    uint32 casdcae = 0;          // WorldUIHpSprite 셰이더에서 0=배경, 1=채움 역할 플래그
    uint32 PassCustomIndex = 0;

    float HpBarAnchorWorldX = 0.f;
    float HpBarAnchorWorldY = 0.f;
    float HpBarAnchorWorldZ = 0.f;
    float HpBarFollowRatio = 1.f; // r1.w — float3 뒤 빈 lane을 채워 패딩 제거

    float HpBarSizePxX = 0.f;
    float HpBarSizePxY = 0.f;
    float HpBarPivotPxX = 0.f;
    float HpBarPivotPxY = 0.f;

    uint32 HpBarBgTexIdx = 0;
    uint32 HpBarFillTexIdx = 0;
    uint32 HpBarHitTexIdx = 0;   // 0이면 hit effect 비활성
    uint32 HpBarHitConfig = 0;   // packed: cols(0..7) | rows(8..15) | frameCount(16..31)
};
static_assert(sizeof(GlobalParamsLayout) == 16 * 4, "GlobalParamsLayout must be 16 DWORDs");

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
