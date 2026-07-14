#pragma once
#include "Entity.h"
#include <string>

class World;

namespace DecalFactory
{
    // 링/원반 :lifetime < 0 이면 무한(수동 제거)
    Entity SpawnRing(World* world, const Vec3& center, float radius,
                     const Vec4& color, float thickness = 0.3f, float lifetime = -1.0f);

    // 스티커
    Entity StampSurfaceSticker(World* world, const Vec3& camPos, const Vec3& forward,
                               const std::wstring& texName, float size = 100.0f,
                               float lifetime = -1.0f);
}
