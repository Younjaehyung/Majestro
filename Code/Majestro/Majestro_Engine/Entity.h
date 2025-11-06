#pragma once

using EntityID = uint32;
constexpr uint32 NULL_ENTITY = 0;

class Entity {
public:
    Entity() : mID(NULL_ENTITY) {}
    Entity(EntityID entityID) : mID(entityID) {}

    EntityID GetID() const { return mID; }
    bool IsValid() const { return mID != NULL_ENTITY; }

    // 비교 연산자
    bool operator==(const Entity& other) const { return mID == other.mID; }
    bool operator!=(const Entity& other) const { return mID != other.mID; }
    bool operator<(const Entity& other) const { return mID < other.mID; }

private:
    EntityID mID;
};
