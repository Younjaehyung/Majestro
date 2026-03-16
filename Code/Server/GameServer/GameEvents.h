#pragma once
#include <variant>
#include <cstdint>
#include "Entity.h"

enum class SkillType : uint8;
enum class EffectSpawnReason : uint8
{
    Fire = 0,
    CollisionEntity = 1,
    CollisionStatic = 2,
    LifetimeExpired = 3,
};

struct EvDamage
{
    Entity target;
    int32 amount;
    Entity instigator; // 0이면 환경
};

struct EvDespawn
{
    Entity target;
};

struct EvSpawnRequest
{
    uint32 netId;
    uint16 prefabType;
    float x, y, z;
};

struct EvNetRPC // 예: 대시 시작 같은 즉시성 이벤트
{
    Entity source;
    uint16 rpcId;
    uint32 payload; // 필요하면 바이트로 확장
};

struct EvHealthChanged
{
    Entity target;
    int32 currentHp;
    int32 maxHp;
};

struct EvArmorChanged
{
    Entity target;
    int32 currentArmor;
    int32 maxArmor;
};

struct EvBulletDeactivated
{
    Entity bullet;
};

struct EvEffectSpawn
{
    uint8 effectType = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    EffectSpawnReason reason = EffectSpawnReason::Fire;
};

struct EvBuffBulletRequest
{
    Entity shooter;
    SkillType bulletType;
};

struct EvRangedAttackRequest
{
    Entity shooter;
    SkillType bulletType;
};

struct EvMeleeAttackRequest
{
    Entity shooter;
    SkillType bulletType;
};

using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvHealthChanged, EvArmorChanged, EvBulletDeactivated, EvEffectSpawn, EvBuffBulletRequest, EvRangedAttackRequest, EvMeleeAttackRequest>;