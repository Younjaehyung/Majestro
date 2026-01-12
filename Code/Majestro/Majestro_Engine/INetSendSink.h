#pragma once
#include "PacketHelper.h"
class INetSendSink
{
public:
    virtual ~INetSendSink() = default;
    virtual bool Enqueue(const SendRequest& req) = 0;
};


