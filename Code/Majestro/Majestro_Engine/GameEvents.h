#pragma once
#include <variant>
#include <cstdint>
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


struct EvBulletCountChanged {
    int current{};
    int max{};
};

struct EvBeat {
    int beat; // 현재 박자 번호
};

struct EvRhythmChanged {
	uint8 musicNum; // 리듬변경이 발생한 박자 번호
};

using GameEvent = std::variant<EvDamage, EvDespawn, EvSpawnRequest, EvNetRPC, EvBulletCountChanged, 
    EvRhythmChanged>;
