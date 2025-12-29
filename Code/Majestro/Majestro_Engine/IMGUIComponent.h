#pragma once
#include "Component.h"

enum class PropertyType
{
	Bool,
	Int,
	Float,
	Vec3,
	Color,
};


struct EditorProperty
{
	std::string name;
	PropertyType type;
	void* data;              // 실제 변수 주소
	float min = 0.0f;
	float max = 0.0f;
};



class IMGUIComponent : public Component<IMGUIComponent>
{
public:
	IMGUIComponent() = default;
	virtual ~IMGUIComponent() = default;
	virtual void SetName(const std::string& name) { mName = name; }
	virtual const char* GetName() const { return mName.c_str(); }
	virtual void RegisterEditorProperties(std::vector<EditorProperty>& props) {}
public:
	std::string mName = "IMGUIComponent";
};

