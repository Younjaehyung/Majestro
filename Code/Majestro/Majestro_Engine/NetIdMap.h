#pragma once
#include "Entity.h"
#include "EnginePch.h"
#include <unordered_map>

class NetIdMap
{
public:
    Entity GetOrInvalid(SceneId sceneId, uint64 netId) const
    {
        //auto it = mMap.find(netId);
        //return (it == mMap.end()) ? NULL_ENTITY : it->second;
        auto sceneIt = mSceneMaps.find(sceneId);
        if (sceneIt == mSceneMaps.end())
            return NULL_ENTITY;

        const auto& sceneMap = sceneIt->second;
        auto it = sceneMap.find(netId);
        return (it == sceneMap.end()) ? NULL_ENTITY : it->second;
    }

    void Bind(SceneId sceneId, uint64 netId, Entity e) { mSceneMaps[sceneId][netId] = e; }
    void Unbind(SceneId sceneId, uint64 netId)
    {
        auto sceneIt = mSceneMaps.find(sceneId);
        if (sceneIt == mSceneMaps.end())
            return;
        sceneIt->second.erase(netId);
    }
    
    void ClearScene(SceneId sceneId) { mSceneMaps.erase(sceneId); }
    void ClearAll() { mSceneMaps.clear(); }
    

private:
    struct SceneIdHash
    {
        size_t operator()(SceneId sceneId) const noexcept
        {
            return std::hash<uint8>{}(static_cast<uint8>(sceneId));
        }
    };

    std::unordered_map<SceneId, std::unordered_map<uint64, Entity>, SceneIdHash> mSceneMaps;
};

