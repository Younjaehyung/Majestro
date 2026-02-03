#pragma once
#include "System.h"
#include "NetIdMap.h"

class  EventManager;

enum class InputButtons : uint8 {
    NONE,
    SPACE,
    SHIFT,
    Q,
    E,
    ATTACK,
    END
};

enum class InputMouse : uint8 {
    NONE,
    LEFT,
    RIGHT,
    WHEEL,
    END
};

class NetSendSystem : public System
{
public:
	NetSendSystem(World* world, EventManager* event);
	void Update(double deltaTime);
    void QueueGameStart();

    void SetCachedPlayerType(uint8 playerType){mCachedPlayerType = playerType;}

public:
	void ConvertInput(SendRequest* seq);
    void SendSceneChange(SceneId targetScene);
private:
    void TrySendGameStart();
    void UpdateCachedPlayerType();
	C2S_InputPacket mInputPacket{};
    bool mHasSentGameStart = false;
    bool mPendingGameStart = false;
    uint8 mCachedPlayerType = 0;

};

