#include "pch.h"
#include "EventManager.h"

void EventManager::BeginPhase(EventPhase phase)
{
    if (phase == EventPhase::Pre)
    {
        mPreRead.clear();
        mPreRead.swap(mPreWrite);
    }
    else
    {
        mPostRead.clear();
        mPostRead.swap(mPostWrite);
    }
}

bool EventManager::Pop(EventPhase phase, GameEvent& out)
{
    auto& read = (phase == EventPhase::Pre) ? mPreRead : mPostRead;
    if (read.empty()) return false;

    out = std::move(read.back());
    read.pop_back();
    return true;
}

