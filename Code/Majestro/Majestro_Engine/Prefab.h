#pragma once
#include "Object.h"

class World;

class Prefab : public Object
{
public:
	Prefab();
	virtual ~Prefab();
public:
	bool mIsRootPrefab = false;
};

class PlayerPrefab : public Prefab
{
public:
	PlayerPrefab(shared_ptr<World> world);
	~PlayerPrefab();
};

