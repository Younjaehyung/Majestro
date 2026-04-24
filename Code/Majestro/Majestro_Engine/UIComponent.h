#pragma once
#include "Entity.h"
#include "Component.h"
#include <string>
#include <vector>

class Mesh;
class Material;

// UIScriptComponent.h
class UIScriptComponent : public Component<UIScriptComponent>
{
public:
	std::function<void(float dt)> mOnUpdate;  // 매 프레임 호출
	std::function<void()>         mOnInit;    // 최초 1회
};

struct UIHpLossFragment
{
	// HP UI 전용 파편 데이터.
	// 월드 메쉬를 만들지 않고 잘린 텍스처 조각만 잠깐 렌더한다.
	Vec2 Center = Vec2::Zero;
	Vec2 Velocity = Vec2::Zero;
	Vec2 Size = Vec2::Zero;
	float Rotation = 0.f;
	float AngularVelocity = 0.f;
	float Age = 0.f;
	float LifeTime = 0.935f;
	RECT SourceRect{ 0, 0, 0, 0 };
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
	bool mHasPreviousHpRatio = false;
	float mPreviousHpRatio = 1.f;
	std::vector<UIHpLossFragment> mLossFragments;
	float mFragmentLifeTime = 0.35f;
	int mMaxFragmentCount = 24;
};
