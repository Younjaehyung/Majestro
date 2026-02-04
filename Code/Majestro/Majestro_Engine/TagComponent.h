#pragma once
#include "Component.h"
#include "Entity.h"


class TagComponent : public Component<TagComponent>
{
public:
	std::string mName;
	TagComponent(std::string& name) : mName(name) {};
};

class MainCameraComponent : public Component<MainCameraComponent> 
{

};

class LocalPlayerComponent : public Component<LocalPlayerComponent>
{
};

class ChoicePlayerComponent : public Component<ChoicePlayerComponent>
{
public:
	ChoicePlayerComponent(uint8 playerType) : mPlayerType(playerType) {};

	uint8 mPlayerType;
};

class MannequinComponent : public Component<MannequinComponent>
{
};