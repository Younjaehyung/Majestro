#pragma once
#include "Component.h"



struct LobbyRoomListEntry
{
    uint32 roomId = 0;
    uint8  playerCount = 0;
    uint8  maxPlayers = 0;
    uint8  phase = 0;          // 0=Waiting, 1=InGame
};

class LobbyRoomListComponent : public Component<LobbyRoomListComponent>
{
public:
    std::array<LobbyRoomListEntry, ROOM_LIST_MAX_ENTRIES> mEntries{};
    uint8  mCount = 0;

    //   mCurrentRoomId == 0  : 방 목록 상태
    //   mCurrentRoomId != 0  : 게임 방 대기 상태
    uint32 mCurrentRoomId = 0;   // JoinResult 성공 시 설정
    bool   mHasList = false;     // S2C_ROOM_LIST 를 한 번이라도 받았는가
};
