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
public:
	MannequinComponent(uint8 playerType) : mPlayerType(playerType) {};

	uint8 mPlayerType;
	bool choice;
};

enum class UITag : uint8
{
	None,
	BulletCount,

};

class UIComponent : public Component<UIComponent>
{
public:
	UIComponent() = default;
	UIComponent(UITag& tag) : mTag(tag) {};
	UITag mTag = UITag::None;
};