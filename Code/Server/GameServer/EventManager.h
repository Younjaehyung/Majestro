#pragma once
#include "GameEvents.h"
#include <vector>

enum class EventPhase : uint8 { Pre, Post };

struct IEventBuffer
{
    virtual ~IEventBuffer() = default;

    // [중요] 프레임(또는 단계) 경계에서 swap
    virtual void Swap() = 0;

    // read 큐 비우기(원하면)
    virtual void ClearRead() = 0;
};

template<typename T>
struct EventBuffer final : IEventBuffer
{
    std::vector<T> read;
    std::vector<T> write;
    void Swap()
    {
        read.clear();
        read.swap(write);
    }

    void ClearRead()
    {
        read.clear();
    }
};

class EventManager
{
public:
    EventManager() = default;

    template<typename T>
    void Enqueue(const T& e);

    template<typename T, typename Fn>
    void Consume(Fn&& fn);

    void SwapBuffers()
    {
        for (auto& [_, ptr] : mBuffers)
            ptr->Swap();
    }

    void ClearAllRead()
    {
        for (auto& [_, ptr] : mBuffers)
            ptr->ClearRead();
    }

private:
    template<typename T>
    EventBuffer<T>& GetBuffer();

private:
    std::unordered_map<std::type_index, std::unique_ptr<IEventBuffer>> mBuffers;
};

template<typename T>
void EventManager::Enqueue(const T& e)
{
    GetBuffer<T>().write.push_back(e);
}

// em.Consume<ContactEvent>([&](const ContactEvent& e) {}
template<typename T, typename Fn>
void EventManager::Consume(Fn&& fn)
{
    auto& buf = GetBuffer<T>();
    for (auto& e : buf.read)
        fn(e);
}

template<typename T>
EventBuffer<T>& EventManager::GetBuffer()
{
    const std::type_index key = typeid(T);

    auto it = mBuffers.find(key);
    if (it == mBuffers.end())
    {
        auto owned = std::make_unique<EventBuffer<T>>();
        auto* raw = owned.get();
        mBuffers.emplace(key, std::move(owned));
        return *raw;
    }
    return *static_cast<EventBuffer<T>*>(it->second.get());
}