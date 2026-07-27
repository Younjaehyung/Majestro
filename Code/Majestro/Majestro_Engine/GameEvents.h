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
    int32 hp;          // 새 HP
    int32 maxHp;       // 최대 HP
    int32 previousHp;  // 이전 HP
    bool  isCritical{}; // 서버 권위 크리티컬 여부

    // 0 벡터면 없음, 피격시 존재
    Vec3 hitDirection{ 0.f, 0.f, 0.f };
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

// 박자 입력 판정 결과
struct EvBeatJudgement {
    uint8 judgement{};    // BeatJudgement (0=Miss,1=Good,2=Perfect)
    uint8 actionButton{};
    bool  predicted{};      // true 는 클라 즉시 1차 판정(반응성), false 는 서버 권위 확정.
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

// 벽 스티커
struct EvStickerRequest
{
    Vec3   pos{};             // 카메라 원점
    Vec3   dir{};             // 시선 방향
    float  size = 100.f;
    uint32 textureId = 0;     // 스티커 종류
};

// 감정표현 선택
struct EvEmoteSelected
{
	uint8 emoteId = 0;
};

// 감정표현 표시
struct EvEmoteTriggered
{
	Entity caster = NULL_ENTITY;
	uint8 emoteId = 0;
};

// 보스 장판(체크무늬 타일) 갱신
struct EvBossTileUpdate
{
    SkillType skillType = SkillType::Default;
    uint8 phase = 0;        // BossTilePhase
    uint8 parity = 0;
    uint8 columnCount = 0;
    uint8 rowCount = 0;
    float originX = 0.0f;
    float originY = 0.0f;   // 장판이 깔리는 바닥 높이 (위층 타일 제외용)
    float originZ = 0.0f;
    float tileSize = 0.0f;
    float durationSec = 0.0f;
};

struct EvVfxSpawnRequest
{
    SkillType skillType = SkillType::Default;
    uint8 reason = 0;
    Vec3 position{};
    Vec3 rotation{};
    uint64 casterNetId = 0;     // 0이면 시전자 없음(고정 VFX). 그 외면 추종 대상 NetworkID
};

struct EvEnemyAttackDebug
{
    SkillType skillType = SkillType::Default;
    uint8 reason = 0;
    Vec3 position{};
    Vec3 rotation{};
    bool positionIsCenter = false;
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


struct EvRoomCreate {};                         // 새 방 생성 요청

struct EvRoomJoin
{
    uint32 roomId;              // 입장할 방
};

struct EvRoomListRequest {};                    // 방 목록 새로고침 요청

struct EvRoomLeave
{
    uint32 roomId;              // 나갈 방 (현재 방)
};


struct EvSfxRequest
{
    std::string sfxKey;         // SfxTable.json 의 키
    Vec3 position{};            // Zero 면 2D 재생
};

// 결과판(Result)
struct EvResultBoardShow
{
    uint8 phase{}; // WavePhaseType (Clear 또는 Fail)
};

// 씬 진입 연출
struct EvIntroFinished {};


using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvBulletCountChanged,
    EvRhythmChanged, EvHpArmorChanged, EvHitMarker, EvVfxSpawnRequest, EvAttachBulletVfx, EvGamePhaseChanged,
    EvRoomReadyChanged, EvRoomCharacterChanged, EvRoomError,
    EvRoomCreate, EvRoomJoin, EvRoomListRequest, EvRoomLeave, EvBeatJudgement, EvIntroFinished>;
