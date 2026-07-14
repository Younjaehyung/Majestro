#pragma once
#include "Entity.h"
#include <string>

class World;

namespace EmoteSticker
{
    // UI_Spray_Sheet.png
    constexpr int    kAtlasCols  = 5;                      // 열 수
    constexpr int    kAtlasRows  = 2;                      // 행 수
    constexpr uint32 kAtlasCount = kAtlasCols * kAtlasRows; // 전체 셀 수 (10)
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
                               int atlasCols = 1, int atlasRows = 1, int atlasIndex = 0);

    // 지면 균열
    Entity StampGroundCrack(World* world, const Vec3& hitPoint, float radius,
                            const std::wstring& texName = L"",
                            const Vec4& color = Vec4(2.0f, 1.2f, 0.6f, 1.0f),
                            float lifetime = 1.5f,
                            int atlasCols = 1, int atlasRows = 1, int atlasIndex = 0);

    // 총알 흔적
    Entity StampBulletHole(World* world, const Vec3& hitPoint, const Vec3& travelDir,
                           const std::wstring& texName = L"", float size = 40.0f,
                           const Vec4& color = Vec4(0.1f, 0.1f, 0.1f, 0.9f),
                           float lifetime = 12.0f,
                           int atlasCols = 1, int atlasRows = 1, int atlasIndex = 0);

    // 근접 흔적
    Entity StampForwardWallMark(World* world, const Vec3& origin, const Vec3& forward,
                                float reachBack, float reachForward, float size,
                                const std::wstring& texName = L"",
                                const Vec4& color = Vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                float lifetime = 12.0f,
                                int atlasCols = 1, int atlasRows = 1, int atlasIndex = 0);
}
