#pragma once
#include "System.h"
#include "NetIdMap.h"
#include "PacketHelper.h"

class  EventManager;

class NetRecvSystem : public System
{
public:
	NetRecvSystem(World* world, EventManager* event,shared_ptr<NetIdMap>& netIdMap);
	virtual ~NetRecvSystem();

	void Initialize();
	void Update(double deltaTime);

	void ProcessOne(const InputCommand& msg);
public:

	void HandleSpawn(const InputCommand& msg);
	void HandleDespawn(const InputCommand& msg);
	void HandleReplicationDelta(const InputCommand& msg);
private:
	//void ApplyNetTransformDelta(PacketReader& r, Entity e, uint32_t fieldMask);
	//void ApplyNetHealthDelta(PacketReader& r, Entity e, uint32_t fieldMask);

	Entity CreateEntityFromArchetype(uint32_t archetypeId);
private:
	shared_ptr<NetIdMap> mNetIdMap = nullptr;

	InputCommand mInputCommand{};
	
	bool mIsPlayer = false;
};

