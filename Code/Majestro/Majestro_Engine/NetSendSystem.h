#pragma once
#include "System.h"
#include "NetIdMap.h"

class  EventManager;

enum class InputButtons : uint32 {
    NONE,
    SPACE,
    SHIFT,
    Q,
    E,
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

