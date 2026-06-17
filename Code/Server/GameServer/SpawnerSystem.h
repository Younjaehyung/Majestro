#pragma once
#include "System.h"
#include <vector>

class World;

class SpawnerSystem : public System
{
public:
    SpawnerSystem(World* world);

    void Initialize() override {}
    void Update(float deltaTime) override;

private:
    void PruneDead(class SpawnerComponent* sp);
    void ResetSpawnPlan(class SpawnerComponent* sp);
    bool PrepareNextSpawnEntry(class SpawnerComponent* sp);
    bool SelectNextSpawnTable(class SpawnerComponent* sp);
    bool CurrentSpawnPlanHasRemainingEntries(const class SpawnerComponent* sp) const;

    class Entity SpawnOne(class Entity spawnerEntity, class SpawnerComponent* sp);

    void BroadcastSpawnPacket(class Entity spawnedEntity, uint8 prefabTypeRaw);

    std::vector<uint32> CollectPlayerSessions() const;

    void ConsumeExternalTriggers();
};
