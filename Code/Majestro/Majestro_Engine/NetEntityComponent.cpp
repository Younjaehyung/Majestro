#include "pch.h"
#include "NetEntityComponent.h"
#include "World.h"

NetEntityComponent::NetEntityComponent()
{
	
}

NetEntityComponent::NetEntityComponent(shared_ptr<World> world, Entity entity)
{
	mNetEntityId = ++NetEntityId;
	mOwnerEntity = entity;
	world->NetIdBinding(mNetEntityId, entity);
}
