#pragma once
#include "System.h"
#include "EventManager.h"

class SystemManager
{
public:
    SystemManager(World* world, EventManager* eventManager): mWorld(world), mEventManager(eventManager) {
    }

    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of_v<System, T>, "T must inherit from BaseSystem");

        auto system = std::make_unique<T>(mWorld, std::forward<Args>(args)...);
        T* systemPtr = system.get();

        mSystems.emplace_back(std::move(system));
        mSystemMap[typeid(T).hash_code()] = systemPtr;

        systemPtr->Initialize();
        return systemPtr;
    }

    template<typename T>
    T* GetSystem() {
        size_t typeHash = typeid(T).hash_code();
        auto it = mSystemMap.find(typeHash);

        if (it != mSystemMap.end()) {
            return static_cast<T*>(it->second);
        }

        return nullptr;
    }

    void Update(float deltaTime) {
        for (auto& sys : mAwakeSystems)        sys->Update(deltaTime);
        for (auto& sys : mStartSystems)        sys->Update(deltaTime);
        for (auto& sys : mUpdateSystems)       sys->Update(deltaTime);
        for (auto& sys : mLateUpdateSystems)   sys->Update(deltaTime);
        for (auto& sys : mFinalUpdateSystems)  sys->Update(deltaTime);
    }

    void Render() {
        for (auto& sys : mRenderSystems)        sys->Update();
    }

    void Shutdown() {
        for (auto& sys : mAwakeSystems)         sys->Shutdown();
        for (auto& sys : mStartSystems)         sys->Shutdown();
        for (auto& sys : mUpdateSystems)        sys->Shutdown();
        for (auto& sys : mLateUpdateSystems)    sys->Shutdown();
        for (auto& sys : mFinalUpdateSystems)   sys->Shutdown();
        for (auto& sys : mRenderSystems)        sys->Shutdown();



        mAwakeSystems.clear();
        mStartSystems.clear();
        mUpdateSystems.clear();
        mUpdateSystems.clear();
        mFinalUpdateSystems.clear();
        mRenderSystems.clear();

        mSystemMap.clear();
    }

private:
    World* mWorld;
    EventManager* mEventManager;
    std::vector<std::unique_ptr<System>> mSystems;

    std::unordered_map<size_t, System*> mSystemMap;

    std::vector<std::unique_ptr<System>> mAwakeSystems;
    std::vector<std::unique_ptr<System>> mStartSystems;
    std::vector<std::unique_ptr<System>> mUpdateSystems;
    std::vector<std::unique_ptr<System>> mLateUpdateSystems;
    std::vector<std::unique_ptr<System>> mFinalUpdateSystems;
    std::vector<std::unique_ptr<System>> mRenderSystems;

};

