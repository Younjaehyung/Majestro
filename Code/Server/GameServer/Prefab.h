#pragma once
#include "Object.h"
#include <array>
#include <cstdint>
#include "Entity.h"
#include "PacketHelper.h"
class World;
class PlayerPrefab;
class TerrainPrefab;
class EnemyPrefab;


class Prefab : public Object
{
public:
	Prefab();
	virtual ~Prefab();

	virtual Entity GetEntityID() { return mEntityID; }
protected:
	bool mIsRootPrefab = true;
	Entity mEntityID;
};


class PrefabFactory
{
public:
	using BuildFn = Entity(*)(World*, const InputCommand&);

	static void RegisterAllPrefabs()
	{
		Register<PrefabType::PLAYER, PlayerPrefab>();
		Register<PrefabType::TERRAIN, TerrainPrefab>();
		Register<PrefabType::ENEMY, EnemyPrefab>();
		// PrefabFactory::Register<PrefabType::SKY_BOX, SkyBoxPrefab>();
		// PrefabFactory::Register<PrefabType::DIR_LIGHT, DirLightPrefab>();
	}


	template<PrefabType Type, typename PrefabT>
	static void Register()
	{
		constexpr size_t idx = static_cast<size_t>(Type);
		sTable[idx] = &PrefabFactory::BuildThunk<PrefabT>;
	}


	static Entity Spawn(World* world, PrefabType type, const InputCommand& ctx)
	{
		const size_t idx = static_cast<size_t>(type);
		if (idx >= sTable.size() || sTable[idx] == nullptr)
			return Entity{}; // invalid

		return sTable[idx](world, ctx);
	}

private:
	template<typename PrefabT>
	static Entity BuildThunk(World* world, const InputCommand& ctx)
	{
		return PrefabT::Build(world, ctx);
	}

private:

	static inline std::array<BuildFn, static_cast<size_t>(PrefabType::COUNT)> sTable{};
};

//class DirLightPrefab : public Prefab
//{
//public:
//	DirLightPrefab(World* world);
//	~DirLightPrefab();
//
//};

class PlayerPrefab : public Prefab
{
public:
	PlayerPrefab(World* world);
	~PlayerPrefab();
public:
	static Entity Build(World* world, const InputCommand& ctx);
};

class SkyBoxPrefab : public Prefab
{
public:
	SkyBoxPrefab(World* world);
	~SkyBoxPrefab();
	//static Entity Build(World& world, const InputCommand& ctx);
};

class TerrainPrefab : public Prefab
{
public:
	TerrainPrefab(World* world);
	~TerrainPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class EnemyPrefab : public Prefab
{
public:
	EnemyPrefab(World* world);
	~EnemyPrefab();
public:
	static Entity Build(World* world, const InputCommand& ctx);

private:
	static int mSpawnCount;
};
