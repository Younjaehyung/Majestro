#pragma once
#include <variant>
#include <cstdint>
#include "Entity.h"

enum class SkillType : uint8;


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
    float impactX = 0.0f;
    float impactY = 0.0f;
    float impactZ = 0.0f;
    bool hasImpact = false;
    uint8 impactReason = 0;
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

using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvHealthChanged, EvArmorChanged, EvBulletDeactivated, EvBuffBulletRequest, EvRangedAttackRequest, EvMeleeAttackRequest>;