#pragma once
#include <stack>
#include <queue>
#include <mutex>
#include "Packet.h"

class ProcessPacket
{
public:
    ProcessPacket();
    ~ProcessPacket() {}
    void Process(BYTE* buffer, int32 len);
	InputCommand* PopCommand();
public:
    void ProcessSyncPacket(BYTE* buffer, int32 len) {};
    void ProcessInputPacket(BYTE* buffer, int32 len) {};
    void ProcessActionPacket(BYTE* buffer, int32 len) {};
private:
	std::mutex                  mPopMutex;
	std::mutex				    mPushMutex;
    std::queue<InputCommand*>   mCommandQueue;
};

