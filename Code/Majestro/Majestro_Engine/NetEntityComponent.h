#pragma once
#include "Component.h"

static uint64 NetEntityId = 0;

class NetEntityComponent : public Component<NetEntityComponent>
{
	enum class Replicationmode : uint8
	{
		NONE = 0,	//직접
		INTERP,		//보간
		PREDICT,	//예상
	};


public:
	uint64 mNetEntityId = ++NetEntityId;
	Replicationmode mNetworkmode = Replicationmode::NONE;

};

