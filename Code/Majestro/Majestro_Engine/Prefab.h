#pragma once
#include "Object.h"
#include "Entity.h"
class World;

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

class DirLightPrefab : public Prefab
{
public:
	DirLightPrefab(shared_ptr<World> world);
	~DirLightPrefab();

};

class PlayerPrefab : public Prefab
{
public:
	PlayerPrefab(shared_ptr<World> world);
	~PlayerPrefab();
};

class SkyBoxPrefab : public Prefab
{
public:
	SkyBoxPrefab(shared_ptr<World> world);
	~SkyBoxPrefab();
};

class TerrainPrefab : public Prefab
{
public:
	TerrainPrefab(shared_ptr<World> world);
	~TerrainPrefab();
};
