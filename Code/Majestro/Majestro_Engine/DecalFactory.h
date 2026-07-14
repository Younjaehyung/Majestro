#pragma once
#include "Entity.h"
#include <string>

class World;

namespace EmoteSticker
{
    constexpr int    kAtlasGrid = 4;                         // 한 변의 셀 수 (4x4)
    constexpr uint32 kAtlasCount = kAtlasGrid * kAtlasGrid; // 전체 이모트 수 (16)
}

namespace DecalFactory
{
    // 링/원반 :lifetime < 0 이면 무한(수동 제거)
    Entity SpawnRing(World* world, const Vec3& center, float radius,
                     const Vec4& color, float thickness = 0.3f, float lifetime = -1.0f);

    // 스티커
    Entity StampSurfaceSticker(World* world, const Vec3& camPos, const Vec3& forward,
                               const std::wstring& texName, float size = 100.0f,
                               float lifetime = -1.0f,
                               int atlasGrid = 1, int atlasIndex = 0);

    // 지면 균열
    Entity StampGroundCrack(World* world, const Vec3& hitPoint, float radius,
                            const std::wstring& texName = L"",
                            const Vec4& color = Vec4(2.0f, 1.2f, 0.6f, 1.0f),
                            float lifetime = 1.5f);
}
