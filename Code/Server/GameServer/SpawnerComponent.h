#pragma once
#include "Component.h"
#include "Entity.h"
#include "PacketHelper.h"
#include <string>

struct SpawnTableEntry
{
    uint8 enemyType = 0;
    int32 count = 0;
};

enum class SpawnerTrigger : uint8
{
    Timed = 0,         // 고정 간격 자동 스폰
    Wave,              // WaveGameMode 등 외부가 명시적으로 활성화해야 동작 (기본 inactive)
    OnPlayerEnter,     // 외부 InteractableComponent가 EvInteractableConsumed 발행 시 활성화 (기본 inactive)
};

class SpawnerComponent : public Component<SpawnerComponent>
{
public:
    SpawnerComponent() = default;

    // 스폰을 시작/유지
    SpawnerTrigger mTrigger = SpawnerTrigger::Timed;

    // 스포너 목적
    PrefabType mPrefabType = PrefabType::ENEMY;
    uint8      mEnemyVariant = 0;   // enemy Type

    std::string mSpawnerId;
    std::vector<std::vector<SpawnTableEntry>> mSpawnTablePool;
    int32 mSelectedTableIndex = -1;
    std::vector<SpawnTableEntry> mCurrentSpawnPlan;
    size_t mCurrentSpawnPlanIndex = 0;
    int32 mCurrentEntryRemaining = 0;
    bool mRandomSpawnFromTable = false;

    // 랜덤 스폰 반경
    float mSpawnRadius = 0.0f;

    float mInterval = 5.0f;                 // 스폰 간격. 0 이하면 즉시 연속 스폰
    float mNextSpawnTime = 0.0f;            // 다음 스폰 가능 절대시간

	
	int32 mTotalSpawned = 0; // 지금까지 누적 스폰된 개체 수 (죽은 것도 포함)
    int32 mMaxTotal = -1;   // -1 : 무한, 0 : 스폰 안 함, 양수 : 최대 누적 스폰 수
    int32 mMaxAlive = 5;   // - 1 : 무한 , 0 : 스폰 안 함, 양수 : 최대 살아있는 개체 수

	std::vector<EntityID> mAliveIds;    // 현재 살아있는 자식 엔티티 ID 리스트

    // 활성 상태: 기본적으로 Timed는 true, Wave/OnPlayerEnter는 false
    bool mActive = true;
};
