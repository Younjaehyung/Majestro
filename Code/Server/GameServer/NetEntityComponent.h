#pragma once
#include "Component.h"
#include "Entity.h"

class NetEntityComponent : public Component<NetEntityComponent>
{
	enum class Replicationmode : uint8
	{
		NONE = 0,	//직접
		INTERP,		//보간
		PREDICT,	//예상
	};


public:
	Entity mOwnerEntity = NULL_ENTITY;
	uint64 mNetEntityId = 0;
	uint32 mSessionId = 0;
	Replicationmode mNetworkmode = Replicationmode::NONE;

};

