#pragma once
#include "Entity.h"
#include "Component.h"
#include <string>

class Mesh;
class Material;


class UIComponent : public Component<UIComponent>
{
public:
	UIComponent() {}

public:
	//Anchor mAnchor = Anchor::TopLeft;


    Vec2 position;     // anchor 기준 오프셋 (pixel)
    Vec2 size;         // pixel
    Vec2 pivot;        // (0~1)
	Vec2 finalPixelPos;   // 최종 화면 픽셀 좌표



	uint8 mUILayerIndex = 0;
	bool mUIVisibility = true;
	vector<shared_ptr<Material>> mMaterials;
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