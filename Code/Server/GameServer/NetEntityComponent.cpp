#include "pch.h"
#include "NetEntityComponent.h"
#include "World.h"


NetEntityComponent::NetEntityComponent()
{
	mNetEntityId = ++NetEntityID;
}

NetEntityComponent::NetEntityComponent(World* world, Entity entity) : NetEntityComponent()
{
	mOwnerEntity = entity;
	world->NetIdBinding(mNetEntityId, entity);
}
