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

    class Entity SpawnOne(class Entity spawnerEntity, class SpawnerComponent* sp);

    void BroadcastSpawnPacket(class Entity spawnedEntity, uint8 prefabTypeRaw);

    std::vector<uint32> CollectPlayerSessions() const;

    void ConsumeExternalTriggers();
};
