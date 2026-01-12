#pragma once
#include "Entity.h"
#include <unordered_map>

class NetIdMap
{
public:
    Entity GetOrInvalid(uint32_t netId) const
    {
        auto it = mMap.find(netId);
        return (it == mMap.end()) ? 0 : it->second;
    }

    void Bind(uint32_t netId, Entity e) { mMap[netId] = e; }
    void Unbind(uint32_t netId) { mMap.erase(netId); }

private:
    std::unordered_map<uint32_t, Entity> mMap;
};

