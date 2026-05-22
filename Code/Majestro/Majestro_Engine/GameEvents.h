#pragma once
#include "Entity.h"

struct EvDamage
{
    Entity instigator; // 공격자/ 0이면 환경
    Entity target;
    int32 amount;
   
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

struct EVContact
{
    Entity a;          // 예: projectile
    Entity b;          // 예: player
    // 필요 시 충돌 지점/노말/penetration 등 추가 가능
};

struct EvNetSceneChange
{
    SceneId targetScene;
};

struct EvHealthChanged
{
    Entity target;
    int32 hp;     // HP 
	int32 maxHp;       // 최대 HP
    // 변화량이 필요하면 여기다가 추가
};

struct EvHpArmorChanged
{
    Entity target;

    int32 armor;  // 방어구 
	int32 maxArmor;     // 최대 방어구
    // 변화량이 필요하면 여기다가 추가
};

struct EvBulletCountChanged {
	Entity target;
    int current{};
    int max{};
};

// 서버가 unicast로 보내는 hit confirm을 받았을 때 발행되는 클라이언트 이벤트.
// HUD 크로스헤어(jAims2) 람다가 단독 소비.
struct EvHitMarker {
    int32 damage{};
    bool  isKill{};
};

struct EvBeat {
    int beat; // 현재 박자 번호
};

struct EvRhythmChanged {
	uint8 musicNum; // 리듬변경이 발생한 박자 번호
};

struct EvSpawnParticleEffect
{
    wstring effectName;
    Vec3 worldPosition{};
    Entity followTarget;
    Vec3 followOffset{};
};

struct EvVfxSpawnRequest
{
    SkillType skillType = SkillType::Default;
    uint8 reason = 0;
    Vec3 position{};
    Vec3 rotation{};
};

struct EvAttachBulletVfx
{
    Entity bullet;
    SkillType skillType = SkillType::Default;
    uint16 generation = 0;
};

struct EvGamePhaseChanged
{
	uint8 prevPhase;
    uint8 newPhase;
};

struct EvRoomReadyChanged
{
    bool ready;                 // 토글한 결과값을 서버로 그대로 전달
};

struct EvRoomCharacterChanged
{
    uint8 playerType;           // ChoicePlayerComponent::mPlayerType 의 신규 값
};


struct EvRoomError
{
    uint8 errorCode;            // RoomErrorCode (Protocol/Packet.h)
};


using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvBulletCountChanged,
    EvRhythmChanged, EvHpArmorChanged, EvHitMarker, EvVfxSpawnRequest, EvAttachBulletVfx, EvGamePhaseChanged,
    EvRoomReadyChanged, EvRoomCharacterChanged, EvRoomError>;
