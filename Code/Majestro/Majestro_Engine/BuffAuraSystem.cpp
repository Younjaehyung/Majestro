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


namespace
{
    constexpr float kRhythmBuffRadius = 1500.f; // 서버 PlayerComponent::mRhythmBuffRadius

    bool ResolveProviderAura(PlayerType type, uint8 rhythm, Vec4& color)
    {
        if (type == PlayerType::Rudwig)
        {
            if (rhythm == 1) { color = Vec4(3.0f, 0.5f, 0.1f, 0.8f); return true; }
            if (rhythm == 3) { color = Vec4(0.2f, 2.2f, 2.6f, 0.8f); return true; }
        }
        else if (type == PlayerType::Ibanix)
        {
            if (rhythm == 1) { color = Vec4(0.2f, 2.2f, 2.6f, 0.8f); return true; }
            if (rhythm == 2) { color = Vec4(0.3f, 0.9f, 3.0f, 0.8f); return true; }
            if (rhythm == 3) { color = Vec4(0.1f, 2.2f, 0.4f, 0.8f); return true; }
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

    const float radius    = kRhythmBuffRadius;
    const float height    = std::max(60.0f, radius * 0.12f);
    const float thickness = std::max(10.0f, radius * 0.02f);

    std::unordered_set<EntityID> activeSet;

    for (Entity player : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        auto* mp = mWorld->GetComponent<MainPlayerComponent>(player);
        auto* tf = mWorld->GetComponent<TransformComponent>(player);
        if (mp == nullptr || tf == nullptr)
            continue;

        // 전환 예약 중이면 다음 리듬으로 판정
        const uint8 rhythm = mp->mHasQueuedRhythmChange ? mp->mNextRhythm : mp->mRhythm;

        Vec4 color;
        bool provides = ResolveProviderAura(mp->mPlayerType, rhythm, color);

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
            aura = DecalFactory::SpawnRing(mWorld, tf->GetWorldPosition(), radius, color, thickness, -1.0f);
            DecalComponent& d = *mWorld->GetComponent<DecalComponent>(aura);
            d.FollowTarget    = player;
            d.NormalThreshold = 0.35f;
        }

        if (auto* d = mWorld->GetComponent<DecalComponent>(aura))
        {
            d->Color     = color;
            d->Radius    = radius;
            d->Height    = height;
            d->Thickness = thickness;

            auto tex = RESOURCEMANAGER.Get<Texture>(L"DecalMagicCircle");
            d->TexIndex = tex ? static_cast<int>(tex->GetImageIndex()) : -1;
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
