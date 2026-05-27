#pragma once
#include "UIFeature.h"

class LobbyRoomListComponent;
class LobbyRoomStateComponent;

// IMGUI 전용
class LobbyRoomBrowserFeature : public UIFeature
{
public:
    void Update(float dt) override;
    void PostSpriteRender(std::vector<UIInstanceData>& instances) override;

private:
    LobbyRoomListComponent*  GetListComp();
    LobbyRoomStateComponent* GetStateComp();
    const char* RoomErrorName(uint8 code) const;

    uint8 mLastErrorCode = 0;   // 0 = 없음
    float mErrorRemain = 0.f;
};
