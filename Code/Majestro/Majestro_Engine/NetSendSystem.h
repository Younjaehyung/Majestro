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
public:
	void ConvertInput(SendRequest* seq);
private:
	C2S_InputPacket mInputPacket{};
};

