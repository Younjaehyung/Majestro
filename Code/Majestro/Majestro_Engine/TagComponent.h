#pragma once
#include "Component.h"
#include "Entity.h"

class Material;

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

	// 로비 캐릭터 머티리얼
	std::vector<std::shared_ptr<Material>> mNormalMaterials; // 선택된 캐릭터 
	std::vector<std::shared_ptr<Material>> mSolidMaterials;		/// mSolidMaterials(타이틀 맵의 Solid 셰이더)
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