#include "pch.h"
#include "BuffAuraSystem.h"

#include "World.h"
#include "DecalComponent.h"
#include "DecalFactory.h"
#include "PlayerComponent.h"      
#include "PlayerStatusComponent.h"
#include "TransformComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Protocol/RhythmDefinitions.h"


namespace
{
    constexpr float kBaseRhythmEffectRadius = 1500.f;

    constexpr const wchar_t* kAuraSheet     = L"UI_Effect_Circle_Sheet";
    constexpr int            kAuraAtlasCols = 2;
    constexpr int            kAuraAtlasRows = 2;

    enum AuraCell : int
    {
        Cell_MoveSpeed = 0,  // 좌상 - 시안
        Cell_Shield    = 1,  // 우상 - 노랑
        Cell_Heal      = 2,  // 좌하 - 초록
        Cell_Attack    = 3,  // 우하 - 빨강
    };

    struct AuraStyle
    {
        Vec4           Color      = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        const wchar_t* Texture    = nullptr;
        int            AtlasIndex = 0;
    };

    bool ResolveProviderAura(PlayerType type, Rhythm rhythm, AuraStyle& out)
    {
        const Vec4 tint(1.0f, 1.0f, 1.0f, 0.55f);

        if (type == PlayerType::Rudwig)
        {
            if (rhythm == Rhythm::R1) { out = { tint, kAuraSheet, Cell_Attack    }; return true; }  // AttackUp
            if (rhythm == Rhythm::R3) { out = { tint, kAuraSheet, Cell_MoveSpeed }; return true; }  // MoveSpeedUp
        }
        else if (type == PlayerType::Ibanix)
        {
            if (rhythm == Rhythm::R1) { out = { tint, kAuraSheet, Cell_MoveSpeed }; return true; }  // MoveSpeedUp10
            if (rhythm == Rhythm::R2) { out = { tint, kAuraSheet, Cell_Shield    }; return true; }  // ShieldOverTime
            if (rhythm == Rhythm::R3) { out = { tint, kAuraSheet, Cell_Heal      }; return true; }  // HealOverTime
        }
        return false;
    }

    bool IsSilenced(World* world, Entity player)
    {
        auto* st = world->GetComponent<PlayerStatusComponent>(player);
        return st && st->FindBuff(ReplicatedBuffType::Silence) != nullptr;
    }
}

void BuffAuraSystem::Update(float /*deltaTime*/)
{
    if (mWorld == nullptr)
        return;

    if (mWorld->HasComponentPool<MainPlayerComponent>() == false)
    {
        for (auto& [pid, aura] : mAuras)
            if (aura.IsValid() && mWorld->HasComponent<DecalComponent>(aura))
                mWorld->DestroyEntity(aura);
        mAuras.clear();
        return;
    }

    const float radius    = kBaseRhythmEffectRadius;
    const float height    = std::max(60.0f, radius * 0.12f);
    const float thickness = std::max(10.0f, radius * 0.02f);

    std::unordered_set<EntityID> activeSet;

    for (Entity player : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        auto* mp = mWorld->GetComponent<MainPlayerComponent>(player);
        auto* tf = mWorld->GetComponent<TransformComponent>(player);
        if (mp == nullptr || tf == nullptr)
            continue;

        // 예약 리듬은 적용 박자 전까지 실제 오라에 반영하지 않는다.
        const Rhythm rhythm = SanitizeRhythm(mp->mRhythm);

        AuraStyle style;
        bool provides = ResolveProviderAura(mp->mPlayerType, rhythm, style);

        if (provides && IsSilenced(mWorld, player))
            provides = false;

        if (provides == false)
            continue;

        activeSet.insert(player.GetID());

        Entity& aura = mAuras[player.GetID()];
        const bool needSpawn = (aura.IsValid() == false) ||
                               (mWorld->HasComponent<DecalComponent>(aura) == false);
        if (needSpawn)
        {
            aura = DecalFactory::SpawnRing(mWorld, tf->GetWorldPosition(), radius, style.Color, thickness, -1.0f);
            DecalComponent& d = *mWorld->GetComponent<DecalComponent>(aura);
            d.FollowTarget    = player;
            d.NormalThreshold = 0.35f;
        }

        if (auto* d = mWorld->GetComponent<DecalComponent>(aura))
        {
            d->Color     = style.Color;
            d->Radius    = radius;
            d->Height    = height;
            d->Thickness = thickness;


            // 텍스처 미지정 능력은 TexIndex = -1 로 두어 절차적 링을 유지한다.
            d->TexIndex = -1;
            if (style.Texture != nullptr)
            {
                auto tex = RESOURCEMANAGER.Get<Texture>(style.Texture);
                if (tex)
                {
                    d->TexIndex   = static_cast<int>(tex->GetImageIndex());
                    d->AtlasGrid  = kAuraAtlasCols;
                    d->AtlasRows  = kAuraAtlasRows;
                    d->AtlasIndex = style.AtlasIndex;
                }
            }
        }
    }

    // 제공 중단 / 플레이어 소멸
    for (auto it = mAuras.begin(); it != mAuras.end(); )
    {
        if (activeSet.find(it->first) == activeSet.end())
        {
            if (it->second.IsValid() && mWorld->HasComponent<DecalComponent>(it->second))
                mWorld->DestroyEntity(it->second);
            it = mAuras.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
