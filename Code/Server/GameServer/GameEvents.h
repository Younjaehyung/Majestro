#pragma once
#include <variant>
#include <cstdint>
#include "Entity.h"

// Fix: SkillType is defined in Protocol/Packet.h and included by pch so packet values cannot drift.

enum class EffectSpawnReason : uint8
{
    Fire = 0,
    CollisionEntity = 1,
    CollisionStatic = 2,
    LifetimeExpired = 3,
};

// 신규 클라이언트가 GAME_START 를 보내 World 에 합류한 직후 발행.
// NetSendSystem 이 소비해서 본인/다른 세션에 spawn 패킷 + 기존 World 스냅샷을 송신한다.
struct EvSessionJoined
{
    uint32 sessionId;
    Entity playerEntity;
    uint8 playerType;
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

struct EvConquestPointCaptured
{
	int currentPointsNum;
    int playerNum;
    int enemyNum;
};

struct EvEscortPointCaptured
{
    int playerNum;
    int enemyNum;
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
    EvHeal, EvImpulse, EvInteractableConsumed, EvHitConfirm, EvConquestPointCaptured, EvEscortPointCaptured>;

