#pragma once
#include "Component.h"
#include "Protocol/RhythmDefinitions.h"


struct LobbyRoomPlayerSlot
{
    uint32 sessionId = 0;
    uint8  playerType = 0;
    RhythmVariantSelection rhythmVariantSelection{};
    bool   ready = false;
    bool   isHost = false;
};

class LobbyRoomStateComponent : public Component<LobbyRoomStateComponent>
{
public:
    uint32 mRoomId = 0;
    uint8  mPlayerCount = 0;
    uint8  mMaxPlayers = 0;
    uint8  mHostSlotIndex = 0xFF;          // 0xFF = Host 없음
    std::array<LobbyRoomPlayerSlot, ROOM_MAX_PLAYERS> mSlots{};
    bool   mHasSnapshot = false;            // S2C_ROOM_STATE 를 한 번이라도 받았는가
};
