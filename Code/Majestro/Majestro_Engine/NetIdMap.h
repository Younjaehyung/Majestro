#pragma once
#include "Entity.h"
#include <unordered_map>

class NetIdMap
{
public:
    Entity GetOrInvalid(uint64 netId) const
    {
        auto it = mMap.find(netId);
        return (it == mMap.end()) ? NULL_ENTITY : it->second;
    }

    void Bind(uint64 netId, Entity e) { mMap[netId] = e; }
    void Unbind(uint64 netId) { mMap.erase(netId); }

private:
    std::unordered_map<uint64, Entity> mMap;
};

