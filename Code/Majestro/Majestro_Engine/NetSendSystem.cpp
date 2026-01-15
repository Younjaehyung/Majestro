#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"

NetSendSystem::NetSendSystem(World* world) : System::System(world)
{

}

void NetSendSystem::Update(double deltaTime)
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();

	for(auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		if (netComp->mIsDirty)
		{
			// Send data through the sink
			
			//gSendBuffer.Push(netComp->mPendingSendData);
			
			netComp->mIsDirty = false;
		}
	}
}