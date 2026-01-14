#pragma once
#include "Component.h"
#include "Entity.h"
static uint64 NetEntityId = 0;


class World;


class NetEntityComponent : public Component<NetEntityComponent>
{
	enum class Replicationmode : uint8
	{
		NONE = 0,	//직접
		INTERP,		//보간
		PREDICT,	//예상
	};
public:
	NetEntityComponent();
	NetEntityComponent(shared_ptr<World> world, Entity entity);


public:
	Entity mOwnerEntity = NULL_ENTITY;
	uint64 mNetEntityId = 0;
	Replicationmode mNetworkmode = Replicationmode::NONE;

};

