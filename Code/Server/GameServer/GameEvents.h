#pragma once
#include "Entity.h"


enum class ImpulseSource : uint8
{
    JumpPad,
    Knockback,
    Wind,
    Conveyor,
    Script
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
    SkillType skillType = SkillType::Default;
    bool isCritical = false;
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

struct EvPlayerDeathRequest
{
    Entity target;
    int8 cause;
    float holdSeconds;
    bool  healthZeroEvent;
};

struct EvHealthChanged
{
    Entity target;
    int32 currentHp;
    int32 maxHp;
    Entity instigator{}; // 어택커 엔티티. 무효 엔티티면 hit이 아닌 변동(힐, DOT, 환경)
    bool isCritical = false;
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

struct EvCooldownStarted
{
    Entity target;
    uint8  skillSlot;        // 0=Attack,1=Skill1,2=Skill2,3=Reload
    float  durationSeconds;  // = Beat * cool
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
    bool isCritical = false;
};

struct EvMeleeAttackRequest
{
    Entity shooter;
    SkillType bulletType;
    bool isCritical = false;
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
    float x, y, z;
    ImpulseSource source;
};

// Interactable 엔티티가 소비되었음을 알리는 이벤트.
struct EvInteractableConsumed
{
    Entity trigger;
    Entity user;
};

// Interactable(힐팩 등)의 표시 상태
struct EvInteractableStateChanged
{
    Entity trigger;
    bool active;    // active=false : 먹혀서 숨김, active=true : 쿨다운 종료로 재등장.
};

// 특정 플레이어의 리듬(담당 음악) 변경 — 전체 클라에 브로드캐스트해 원격 음악 동기화.
struct EvRhythmChanged
{
    Entity player;
    uint8 previousRhythm;
    uint8 changedRhythm;
    uint8 playerType;
    int64 applyAtBeatIndex; 
};




using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvHealthChanged, EvArmorChanged, EvAmmoChanged,
    EvBulletDeactivated, EvEffectSpawn, EvBuffRequest, EvRangedAttackRequest, EvMeleeAttackRequest,
    EvHeal, EvImpulse, EvInteractableConsumed, EvInteractableStateChanged, EvHitConfirm, EvConquestPointCaptured, EvEscortPointCaptured, EvPlayerDeathRequest,
    EvCooldownStarted, EvRhythmChanged>;

