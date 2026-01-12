#pragma once

#include "INetSendSink.h"



class MainNetSendSink : public INetSendSink
{
public:
    bool Enqueue(const SendRequest& req) override
    {
        return 0;// return gSendBuffer.Push(req);
    }
    bool Dequeue(SendRequest& out)
    {

        return 0;
    }
};

//class MainNetRecvSink : public INetRecvSink
//{
//    public:
//    bool Enqueue(const InputCommand& cmd) override
//    {
//        return gRecvBuffer.Push(cmd);
//    }
//    bool Dequeue(InputCommand& out)
//    {
//        return 0;
//    }
//};