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

class BackviewCameraComponent : public Component<BackviewCameraComponent>
{

};


