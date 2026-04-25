#pragma once
#include <variant>
#include <cstdint>
#include "Entity.h"

enum class SkillType : uint8
{
    Default = 0,
    BaseAttack,
    BaseSkill1,
    BaseSkill2,
    BaseSkill3,

    GuitarAttack,
    GuitarSkill1,
    GuitarSkill2,
    GuitarSkill3,

    DrumAttack,
    DrumSkill1,
    DrumSkill2,
    DrumSkill3,

    GuitarAttack_1,
    GuitarAttack_2,
    GuitarAttack_3,

    //mop
    HornAttack,
    PianoAttack,
    BongoAttack,

    Max
};

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

struct EvAmmoChanged
{
    Entity target;
    int32 currentAmmo;
    int32 maxAmmo;
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
    float rotX = 0.0f;
    float rotY = 0.0f;
    float rotZ = 0.0f;
};

struct EvBuffRequest
{
    Entity target;
    SkillType skillType;
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

struct EvHeal
{
    Entity target;
    int32 amount;
    Entity instigator;
};

struct EvHitConfirm
{
    Entity instigator;  // instigator의 sessionId 한 곳에만 unicast로 송신
    Entity target;
    int32 damage;
    bool isKill;
};

struct EvImpulse
{
    Entity target;
    float x;
    float y; // 위 방향이 +Y.
    float z;
};

// Interactable 엔티티가 소비되었음을 알리는 이벤트.
struct EvInteractableConsumed
{
    Entity trigger;
    Entity user;
};


using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvHealthChanged, EvArmorChanged, EvAmmoChanged,
    EvBulletDeactivated, EvEffectSpawn, EvBuffRequest, EvRangedAttackRequest, EvMeleeAttackRequest,
    EvHeal, EvImpulse, EvInteractableConsumed, EvHitConfirm>;

