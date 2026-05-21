#pragma once
#include "UIFeature.h"

// 현재 IMGUI로 대체

class LobbyRoomUIFeature : public UIFeature
{
public:
    void Initialize(World* world) override;
    void Update(float dt) override;
    void PostSpriteRender(std::vector<UIInstanceData>& instances) override;

private:
    const char* GetPlayerTypeName(uint8 playerType) const;
    const char* GetRoomErrorName(uint8 code) const;


    uint8 mLastErrorCode = 0;     // 0 = 없음
    float mErrorRemain = 0.f;
};
