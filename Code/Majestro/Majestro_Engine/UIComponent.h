#pragma once
#include "Entity.h"
#include "Component.h"
#include <string>

class Mesh;
class Material;

// UIScriptComponent.h
class UIScriptComponent : public Component<UIScriptComponent>
{
public:
	std::function<void(float dt)> mOnUpdate;  // 매 프레임 호출
	std::function<void()>         mOnInit;    // 최초 1회
};

class UIHpBarComponent : public Component<UIHpBarComponent>
{
public:
	UIHpBarComponent() = default;
	UIHpBarComponent(
		float maxWidth,
		Entity targetEntity = NULL_ENTITY,
		const Vec3& worldOffset = Vec3(0.f, 120.f, 0.f),
		float height = 20.f,
		const std::wstring& backgroundMaterialName = L"HPBAR",
		const std::wstring& fillMaterialName = L"")
		: mMaxWidth((std::max)(0.0f, maxWidth))
		, mTargetEntity(targetEntity)
		, mWorldOffset(worldOffset)
		, mHeight((std::max)(0.0f, height))
		, mBackgroundMaterialName(backgroundMaterialName)
		, mFillMaterialName(fillMaterialName.empty() ? backgroundMaterialName : fillMaterialName) {
	}
public:
	float mMaxWidth = 180.0f;
	float mHeight = 20.0f;
	Entity mTargetEntity = NULL_ENTITY;
	Vec3 mWorldOffset = Vec3(0.f, 120.f, 0.f);
	std::wstring mBackgroundMaterialName = L"HPBAR";
	std::wstring mFillMaterialName = L"HPBAR";

	Entity mBackgroundUIEntity = NULL_ENTITY;
	Entity mFillUIEntity = NULL_ENTITY;
};