#pragma once

#include <vector>
#include <string>
#include "Object.h"

struct PbrSet {
    std::string albedo, normal, metallic, smoothness, occlusion, emission;
};

struct MapObjectDesc {
    std::string name;
    bool active;
    Vec3 pos, rotEulerDeg, scale;
    std::string meshFbxFile;
    std::string meshName;
    std::vector<PbrSet> materials; // slot index = vector index
};

struct SceneMapDesc {
    std::string sceneName;
    std::string bakedBaseDir;
    std::string fbxBaseDir;
    std::vector<MapObjectDesc> objects;
};

static Vec3 ReadVec3(const nlohmann::json& a)
{
    return Vec3{ a[0].get<float>(), a[1].get<float>(), a[2].get<float>() };
}
