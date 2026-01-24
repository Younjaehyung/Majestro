#pragma once
#include "GameEvents.h"
#include <vector>

enum class EventPhase : uint8 { Pre, Post };

class EventManager
{
public:
    void Initialize() {}
    

public:
    void PushPre(GameEvent e) { mPreWrite.emplace_back(std::move(e)); }
    void PushPost(GameEvent e) { mPostWrite.emplace_back(std::move(e)); }

    void BeginPhase(EventPhase phase)
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

    bool Pop(EventPhase phase, GameEvent& out)
    {
        auto& read = (phase == EventPhase::Pre) ? mPreRead : mPostRead;
        if (read.empty()) return false;

        out = std::move(read.back());
        read.pop_back();
        return true;
    }

    // 처리 중 새로 Push된 이벤트를 “같은 phase에서 계속 처리”하고 싶으면,
    //        mPreWrite/mPostWrite를 다시 BeginPhase로 넘겨 read로 옮기면 된다.
    bool HasPendingWrites(EventPhase phase) const
    {
        return (phase == EventPhase::Pre) ? !mPreWrite.empty() : !mPostWrite.empty();
    }

private:
    //현재 phase에서 소비되는 큐
    std::vector<GameEvent> mPreRead, mPostRead;


    // 시스템들이 Push하는 큐(처리 중에도 안전하게 추가 가능)
    std::vector<GameEvent> mPreWrite, mPostWrite;
};
