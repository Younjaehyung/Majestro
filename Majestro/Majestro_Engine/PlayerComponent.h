#pragma once
#include "Component.h"
#include "TransformComponent.h"

class PlayerComponent : public Component<PlayerComponent>
{
public:
	PlayerComponent() {}
	PlayerComponent(TransformComponent transform): mTransformComponent(transform) {}
	PlayerComponent(TransformComponent transform, std::string mode) : mTransformComponent(transform), mPlayMode(mode) {}
public:
	TransformComponent mTransformComponent;
	std::string mPlayMode;
};