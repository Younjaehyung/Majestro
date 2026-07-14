#include "pch.h"
#include "DecalFactory.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "World.h"
#include "DecalComponent.h"

namespace DecalFactory
{
    Entity SpawnRing(World* world, const Vec3& center, float radius,
                     const Vec4& color, float thickness, float lifetime)
    {
        Entity e = world->CreateEntity();

        DecalComponent& decal = world->AddComponent<DecalComponent>(e);
        decal.Center    = center;
        decal.Radius    = radius;
        decal.Thickness = thickness;
        decal.Color     = color;
        decal.Lifetime  = lifetime;
        decal.Elapsed   = 0.0f;

        return e;
    }

    Entity StampSurfaceSticker(World* world, const Vec3& camPos, const Vec3& forward,
                               const std::wstring& texName, float size, float lifetime)
    {
        // 시선 축으로 긴 슬래브를 배치해 뎁스 버퍼의 첫 표면(벽/바닥)에 투영.
        const float reach  = 800.0f;
        const Vec3  center = camPos + forward * reach;

        // 텍스처 없을 때
        Entity e = SpawnRing(world, center, size, Vec4(1.0f, 1.0f, 1.0f, 1.0f), size, lifetime);

        DecalComponent& d = *world->GetComponent<DecalComponent>(e);
        d.Normal          = -forward;   // 표면이 카메라를 향한다고 가정
        d.Height          = reach;      // 슬래브 두께(투영축) 
        d.NormalThreshold = 0.3f;

        if (!texName.empty())
        {
            auto tex = RESOURCEMANAGER.Get<Texture>(texName);
            d.TexIndex = tex ? static_cast<int>(tex->GetImageIndex()) : -1;
        }

        return e;
    }
}
