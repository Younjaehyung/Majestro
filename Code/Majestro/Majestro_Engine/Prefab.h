#pragma once
#include "Object.h"
#include "Entity.h"
#include "PacketHelper.h"
class World;
class PlayerPrefab;
class TerrainPrefab;
class EnemyPrefab;
class BulletPrefab;
class HealPackPrefab;
class JumpPadPrefab;
class MonsterSpawnerMarkerPrefab;
class TruckEscortPrefab;

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
		Register<PrefabType::BULLET, BulletPrefab>();
		Register<PrefabType::HEAL_PACK, HealPackPrefab>();
		Register<PrefabType::JUMP_PAD, JumpPadPrefab>();
		Register<PrefabType::MONSTER_SPAWNER, MonsterSpawnerMarkerPrefab>();
		Register<PrefabType::TRUCK, TruckEscortPrefab>();
		// Register<PrefabType::ENEMY, EnemyPrefab>();
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

	static Entity BuildWorldMarkerPrefab(World* world, const InputCommand& ctx, const wchar_t* effectName, const Vec3& scale);
	

private:
	template<typename PrefabT>
	static Entity BuildThunk(World* world, const InputCommand& ctx)
	{
		return PrefabT::Build(world, ctx);
	}

private:

	static inline std::array<BuildFn, static_cast<size_t>(PrefabType::COUNT)> sTable{};
};

// 맵별 태양 색상. 생략하면 기존 공통값이 그대로 쓰인다.
struct DirLightColors
{
	Vec3 diffuse { 1.0f, 1.0f, 1.0f };
	Vec3 ambient { 0.2f, 0.2f, 0.2f };
	Vec3 specular{ 0.3f, 0.3f, 0.3f };
};

class DirLightPrefab : public Prefab
{
public:
	static constexpr Vec3 kDefaultDirection{ -0.0713f, -0.6448f, 0.7610f };

	DirLightPrefab(World* world,
		const Vec3& direction = kDefaultDirection,
		const DirLightColors& colors = DirLightColors{});
	~DirLightPrefab();

};

class PlayerPrefab : public Prefab
{
public:
	PlayerPrefab(World* world);
	~PlayerPrefab();
public:
	static Entity Build(World* world, const InputCommand& ctx);
};

class EnemyPrefab : public Prefab
{
public:
	EnemyPrefab(World* world);
	~EnemyPrefab();
public:
	static Entity Build(World* world, const InputCommand& ctx);
};

class BulletPrefab : public Prefab
{
public:
	BulletPrefab(World* world);
	~BulletPrefab();
public:
	static Entity Build(World* world, const InputCommand& ctx);
};

class HealPackPrefab : public Prefab
{
public:
	HealPackPrefab(World* world);
	~HealPackPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class JumpPadPrefab : public Prefab
{
public:
	JumpPadPrefab(World* world);
	~JumpPadPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class MonsterSpawnerMarkerPrefab : public Prefab
{
public:
	MonsterSpawnerMarkerPrefab(World* world);
	~MonsterSpawnerMarkerPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class SkyBoxPrefab : public Prefab
{
public:
	SkyBoxPrefab(World* world);
	~SkyBoxPrefab();
	//static Entity Build(World& world, const InputCommand& ctx);
};


class OceanPrefab : public Prefab
{
public:
	OceanPrefab(World* world);
	~OceanPrefab();
};

class BillboardPrefab : public Prefab
{
public:
	BillboardPrefab(World* world);
	~BillboardPrefab();
	//static Entity Build(World& world, const InputCommand& ctx);
};

class TerrainPrefab : public Prefab
{
public:
	TerrainPrefab(World* world);
	~TerrainPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

// 초상화
class HUDPortraitPrefab : public Prefab
{
public:

	HUDPortraitPrefab(World* world, uint8 playerType);
	~HUDPortraitPrefab();

};

// 스킬 쿨타임 바
class HUDSkillBarPrefab : public Prefab
{
public:
	HUDSkillBarPrefab(World* world, uint8 playerType);
	~HUDSkillBarPrefab();

private:

};

class HUDHPBarPrefab : public Prefab
{
public:
	HUDHPBarPrefab(World* world, uint8 playerType, Entity ownerEntity);
	~HUDHPBarPrefab();
};

class HUDBossHPBarPrefab : public Prefab
{
public:
	HUDBossHPBarPrefab(World* world, Entity bossEntity, int bossType);
	~HUDBossHPBarPrefab();
};

class HUDWeaponPrefab : public Prefab
{
public:
	HUDWeaponPrefab(World* world, uint8 playerType, Entity ownerEntity);
	~HUDWeaponPrefab();
};

class HUDMusicPrefab : public Prefab
{
public:
	HUDMusicPrefab(World* world, uint8 playerType, Entity ownerEntity);
	~HUDMusicPrefab();
};

class HUDCrosshairPrefab : public Prefab
{
public:
	HUDCrosshairPrefab(World* world);
	~HUDCrosshairPrefab();
};

class HUDScorePrefab : public Prefab
{
public:
	HUDScorePrefab(World* world);
	~HUDScorePrefab();
};

class HUDTimerPrefab : public Prefab
{
	public:
	HUDTimerPrefab(World* world);
	~HUDTimerPrefab();
};

class AreaConquestPrefab : public Prefab
{
	public:
	AreaConquestPrefab(World* world);
	~AreaConquestPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class AreaEscortPrefab : public Prefab
{
	public:
	AreaEscortPrefab(World* world);
	~AreaEscortPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};

class TruckEscortPrefab : public Prefab
{
public:
	TruckEscortPrefab(World* world);
	~TruckEscortPrefab();
	static Entity Build(World* world, const InputCommand& ctx);
};
