#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "INetSendSink.h"
#include "NetEntityComponent.h"

NetSendSystem::NetSendSystem(World* world, shared_ptr<NetIdMap>& netSendSink) : System::System(world)
{
	mNetSendSink = netSendSink;
}
NetSendSystem::~NetSendSystem()
{
}
void NetSendSystem::Update(double deltaTime)
{
	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();

	for(auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		if (netComp->mIsDirty)
		{
			// Send data through the sink
			if (mNetSendSink)
			{
				//mNetSendSink->Enqueue(SendRequest());
			}
			netComp->mIsDirty = false;
		}
	}
}