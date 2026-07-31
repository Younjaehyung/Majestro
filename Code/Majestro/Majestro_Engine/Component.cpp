#include "pch.h"
#include "Component.h"


ComponentTypeID AllocateComponentTypeID()
{
    static std::atomic<ComponentTypeID> sNextTypeID{ 0 };
    return sNextTypeID.fetch_add(1, std::memory_order_relaxed);
}

BaseComponent::BaseComponent()
{
}
