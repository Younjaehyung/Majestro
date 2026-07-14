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
                               const std::wstring& texName, float size, float lifetime,
                               int atlasGrid, int atlasIndex)
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

        d.AtlasGrid  = (std::max)(atlasGrid, 1);
        d.AtlasIndex = atlasIndex;

        return e;
    }

    Entity StampGroundCrack(World* world, const Vec3& hitPoint, float radius,
                            const std::wstring& texName, const Vec4& color, float lifetime)
    {
        Entity e = world->CreateEntity();

        DecalComponent& d = world->AddComponent<DecalComponent>(e);
        d.Center          = hitPoint + Vec3(0.0f, 20.0f, 0.0f); // 슬래브가 지면을 덮도록 살짝 위
        d.Normal          = Vec3(0.0f, 1.0f, 0.0f);             // 바닥 투영(수직 아래로 찍힘)
        d.Radius          = radius;                              // 면 half-size
        d.Height          = 40.0f;                               // 얇은 슬래브(지면 요철 흡수)
        d.Thickness       = radius * 0.15f;                      // 텍스처 없을 때 링 두께
        d.NormalThreshold = 0.4f;                                // 평지만(경사면 컷)
        d.Color           = color;                               // >1 이면 Bloom 발광
        d.Lifetime        = lifetime;
        d.Elapsed         = 0.0f;
        d.FadeIn          = 0.05f;                               // 착지 순간 팍 등장
        d.FadeOut         = (std::min)(0.8f, lifetime * 0.6f);   // 서서히 소멸

        if (!texName.empty())
        {
            auto tex = RESOURCEMANAGER.Get<Texture>(texName);
            d.TexIndex = tex ? static_cast<int>(tex->GetImageIndex()) : -1;
        }

        return e;
    }
}
